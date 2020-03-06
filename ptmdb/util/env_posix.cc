// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <deque>
#include <limits>
#include <set>

#include "../env.h"
#include "../slice.h"
#include "../status.h"
#define LEVELDB_PLATFORM_POSIX
#include "../port/port_posix.h"
#include "../util/mutexlock.h"

namespace ptmdb {

namespace {

static int open_read_only_file_limit = -1;

static const size_t kBufSize = 65536;

static Status PosixError(const std::string& context, int err_number) {
  if (err_number == ENOENT) {
    return Status::NotFound(context, strerror(err_number));
  } else {
    return Status::IOError(context, strerror(err_number));
  }
}


class PosixSequentialFile: public SequentialFile {
 private:
  std::string filename_;
  int fd_;

 public:
  PosixSequentialFile(const std::string& fname, int fd)
      : filename_(fname), fd_(fd) {}
  virtual ~PosixSequentialFile() { close(fd_); }

  virtual Status Read(size_t n, Slice* result, char* scratch) {
    Status s;
    while (true) {
      ssize_t r = read(fd_, scratch, n);
      if (r < 0) {
        if (errno == EINTR) {
          continue;  // Retry
        }
        s = PosixError(filename_, errno);
        break;
      }
      *result = Slice(scratch, r);
      break;
    }
    return s;
  }

  virtual Status Skip(uint64_t n) {
    if (lseek(fd_, n, SEEK_CUR) == static_cast<off_t>(-1)) {
      return PosixError(filename_, errno);
    }
    return Status::OK();
  }
};



class PosixWritableFile : public WritableFile {
 private:
  // buf_[0, pos_-1] contains data to be written to fd_.
  std::string filename_;
  int fd_;
  char buf_[kBufSize];
  size_t pos_;

 public:
  PosixWritableFile(const std::string& fname, int fd)
      : filename_(fname), fd_(fd), pos_(0) { }

  ~PosixWritableFile() {
    if (fd_ >= 0) {
      // Ignoring any potential errors
      Close();
    }
  }

  virtual Status Append(const Slice& data) {
    size_t n = data.size();
    const char* p = data.data();

    // Fit as much as possible into buffer.
    size_t copy = std::min(n, kBufSize - pos_);
    memcpy(buf_ + pos_, p, copy);
    p += copy;
    n -= copy;
    pos_ += copy;
    if (n == 0) {
      return Status::OK();
    }

    // Can't fit in buffer, so need to do at least one write.
    Status s = FlushBuffered();
    if (!s.ok()) {
      return s;
    }

    // Small writes go to buffer, large writes are written directly.
    if (n < kBufSize) {
      memcpy(buf_, p, n);
      pos_ = n;
      return Status::OK();
    }
    return WriteRaw(p, n);
  }

  virtual Status Close() {
    Status result = FlushBuffered();
    const int r = close(fd_);
    if (r < 0 && result.ok()) {
      result = PosixError(filename_, errno);
    }
    fd_ = -1;
    return result;
  }

  virtual Status Flush() {
    return FlushBuffered();
  }

  Status SyncDirIfManifest() {
    const char* f = filename_.c_str();
    const char* sep = strrchr(f, '/');
    Slice basename;
    std::string dir;
    if (sep == NULL) {
      dir = ".";
      basename = f;
    } else {
      dir = std::string(f, sep - f);
      basename = sep + 1;
    }
    Status s;
    if (basename.starts_with("MANIFEST")) {
      int fd = open(dir.c_str(), O_RDONLY);
      if (fd < 0) {
        s = PosixError(dir, errno);
      } else {
        if (fsync(fd) < 0) {
          s = PosixError(dir, errno);
        }
        close(fd);
      }
    }
    return s;
  }

  virtual Status Sync() {
    // Ensure new files referred to by the manifest are in the filesystem.
    Status s = SyncDirIfManifest();
    if (!s.ok()) {
      return s;
    }
    s = FlushBuffered();
    if (s.ok()) {
      if (fdatasync(fd_) != 0) {
        s = PosixError(filename_, errno);
      }
    }
    return s;
  }

 private:
  Status FlushBuffered() {
    Status s = WriteRaw(buf_, pos_);
    pos_ = 0;
    return s;
  }

  Status WriteRaw(const char* p, size_t n) {
    while (n > 0) {
      ssize_t r = write(fd_, p, n);
      if (r < 0) {
        if (errno == EINTR) {
          continue;  // Retry
        }
        return PosixError(filename_, errno);
      }
      p += r;
      n -= r;
    }
    return Status::OK();
  }
};


static int LockOrUnlock(int fd, bool lock) {
  errno = 0;
  struct flock f;
  memset(&f, 0, sizeof(f));
  f.l_type = (lock ? F_WRLCK : F_UNLCK);
  f.l_whence = SEEK_SET;
  f.l_start = 0;
  f.l_len = 0;        // Lock/unlock entire file
  return fcntl(fd, F_SETLK, &f);
}

// Set of locked files.  We keep a separate set instead of just
// relying on fcntrl(F_SETLK) since fcntl(F_SETLK) does not provide
// any protection against multiple uses from the same process.
class PosixLockTable {
 private:
  port::Mutex mu_;
  std::set<std::string> locked_files_;
 public:
  bool Insert(const std::string& fname) {
    MutexLock l(&mu_);
    return locked_files_.insert(fname).second;
  }
  void Remove(const std::string& fname) {
    MutexLock l(&mu_);
    locked_files_.erase(fname);
  }
};

class PosixEnv : public Env {
 public:
  PosixEnv();
  virtual ~PosixEnv() {
    char msg[] = "Destroying Env::Default()\n";
    fwrite(msg, 1, sizeof(msg), stderr);
    abort();
  }


  virtual Status NewSequentialFile(const std::string& fname,
                                   SequentialFile** result) {
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd < 0) {
      *result = NULL;
      return PosixError(fname, errno);
    } else {
      *result = new PosixSequentialFile(fname, fd);
      return Status::OK();
    }
  }



  virtual Status NewWritableFile(const std::string& fname,
                                 WritableFile** result) {
    Status s;
    int fd = open(fname.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
      *result = NULL;
      s = PosixError(fname, errno);
    } else {
      *result = new PosixWritableFile(fname, fd);
    }
    return s;
  }


  virtual Status NewAppendableFile(const std::string& fname,
                                   WritableFile** result) {
    Status s;
    int fd = open(fname.c_str(), O_APPEND | O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
      *result = NULL;
      s = PosixError(fname, errno);
    } else {
      *result = new PosixWritableFile(fname, fd);
    }
    return s;
  }

  virtual bool FileExists(const std::string& fname) {
    return access(fname.c_str(), F_OK) == 0;
  }

  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) {
    result->clear();
    DIR* d = opendir(dir.c_str());
    if (d == NULL) {
      return PosixError(dir, errno);
    }
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
      result->push_back(entry->d_name);
    }
    closedir(d);
    return Status::OK();
  }

  virtual Status DeleteFile(const std::string& fname) {
    Status result;
    if (unlink(fname.c_str()) != 0) {
      result = PosixError(fname, errno);
    }
    return result;
  }

  virtual Status CreateDir(const std::string& name) {
    Status result;
    if (mkdir(name.c_str(), 0755) != 0) {
      result = PosixError(name, errno);
    }
    return result;
  }

  virtual Status DeleteDir(const std::string& name) {
    Status result;
    if (rmdir(name.c_str()) != 0) {
      result = PosixError(name, errno);
    }
    return result;
  }

  virtual Status GetFileSize(const std::string& fname, uint64_t* size) {
    Status s;
    struct stat sbuf;
    if (stat(fname.c_str(), &sbuf) != 0) {
      *size = 0;
      s = PosixError(fname, errno);
    } else {
      *size = sbuf.st_size;
    }
    return s;
  }

  virtual Status RenameFile(const std::string& src, const std::string& target) {
    Status result;
    if (rename(src.c_str(), target.c_str()) != 0) {
      result = PosixError(src, errno);
    }
    return result;
  }

  virtual void Schedule(void (*function)(void*), void* arg);

  virtual void StartThread(void (*function)(void* arg), void* arg);

  virtual Status GetTestDirectory(std::string* result) {
    const char* env = getenv("TEST_TMPDIR");
    if (env && env[0] != '\0') {
      *result = env;
    } else {
      char buf[100];
      snprintf(buf, sizeof(buf), "/tmp/leveldbtest-%d", int(geteuid()));
      *result = buf;
    }
    // Directory may already exist
    CreateDir(*result);
    return Status::OK();
  }

  static uint64_t gettid() {
    pthread_t tid = pthread_self();
    uint64_t thread_id = 0;
    memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
    return thread_id;
  }

  virtual Status NewLogger(const std::string& fname, Logger** result) {
      printf("ERROR: implement NewLogger in env_posix.cc or add PosixLogger class\n");
      return Status::OK();
    /*
    FILE* f = fopen(fname.c_str(), "w");
    if (f == NULL) {
      *result = NULL;
      return PosixError(fname, errno);
    } else {
      *result = new PosixLogger(f, &PosixEnv::gettid);
      return Status::OK();
    }
    */
  }

  virtual uint64_t NowMicros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
  }

  virtual void SleepForMicroseconds(int micros) {
    usleep(micros);
  }

 private:
  void PthreadCall(const char* label, int result) {
    if (result != 0) {
      fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
      abort();
    }
  }

  // BGThread() is the body of the background thread
  void BGThread();
  static void* BGThreadWrapper(void* arg) {
    reinterpret_cast<PosixEnv*>(arg)->BGThread();
    return NULL;
  }

  pthread_mutex_t mu_;
  pthread_cond_t bgsignal_;
  pthread_t bgthread_;
  bool started_bgthread_;

  // Entry per Schedule() call
  struct BGItem { void* arg; void (*function)(void*); };
  typedef std::deque<BGItem> BGQueue;
  BGQueue queue_;

  PosixLockTable locks_;
};


// Return the maximum number of read-only files to keep open.
static intptr_t MaxOpenFiles() {
  if (open_read_only_file_limit >= 0) {
    return open_read_only_file_limit;
  }
  struct rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim)) {
    // getrlimit failed, fallback to hard-coded default.
    open_read_only_file_limit = 50;
  } else if (rlim.rlim_cur == RLIM_INFINITY) {
    open_read_only_file_limit = std::numeric_limits<int>::max();
  } else {
    // Allow use of 20% of available file descriptors for read-only files.
    open_read_only_file_limit = rlim.rlim_cur / 5;
  }
  return open_read_only_file_limit;
}

PosixEnv::PosixEnv()
    : started_bgthread_(false) {
  PthreadCall("mutex_init", pthread_mutex_init(&mu_, NULL));
  PthreadCall("cvar_init", pthread_cond_init(&bgsignal_, NULL));
}

void PosixEnv::Schedule(void (*function)(void*), void* arg) {
  PthreadCall("lock", pthread_mutex_lock(&mu_));

  // Start background thread if necessary
  if (!started_bgthread_) {
    started_bgthread_ = true;
    PthreadCall(
        "create thread",
        pthread_create(&bgthread_, NULL,  &PosixEnv::BGThreadWrapper, this));
  }

  // If the queue is currently empty, the background thread may currently be
  // waiting.
  if (queue_.empty()) {
    PthreadCall("signal", pthread_cond_signal(&bgsignal_));
  }

  // Add to priority queue
  queue_.push_back(BGItem());
  queue_.back().function = function;
  queue_.back().arg = arg;

  PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

void PosixEnv::BGThread() {
  while (true) {
    // Wait until there is an item that is ready to run
    PthreadCall("lock", pthread_mutex_lock(&mu_));
    while (queue_.empty()) {
      PthreadCall("wait", pthread_cond_wait(&bgsignal_, &mu_));
    }

    void (*function)(void*) = queue_.front().function;
    void* arg = queue_.front().arg;
    queue_.pop_front();

    PthreadCall("unlock", pthread_mutex_unlock(&mu_));
    (*function)(arg);
  }
}

namespace {
struct StartThreadState {
  void (*user_function)(void*);
  void* arg;
};
}
static void* StartThreadWrapper(void* arg) {
  StartThreadState* state = reinterpret_cast<StartThreadState*>(arg);
  state->user_function(state->arg);
  delete state;
  return NULL;
}

void PosixEnv::StartThread(void (*function)(void* arg), void* arg) {
  pthread_t t;
  StartThreadState* state = new StartThreadState;
  state->user_function = function;
  state->arg = arg;
  PthreadCall("start thread",
              pthread_create(&t, NULL,  &StartThreadWrapper, state));
}

}  // namespace

static pthread_once_t once = PTHREAD_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new PosixEnv; }

Env* Env::Default() {
  pthread_once(&once, InitDefaultEnv);
  return default_env;
}

}  // namespace ptmdb
