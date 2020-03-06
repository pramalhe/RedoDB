#ifndef _PTM_DB_INCLUDE_H_
#define _PTM_DB_INCLUDE_H_
// We define here all the conditionals for the PTMs we want to use in our DB.
// We could do all of this stuff with C++ templatization, but it's just simpler to use C macros. Both options are ugly.

#ifdef USE_PTL2_REDO

#elif defined USE_PTL2_UNDO

#elif defined USE_ROMULUS_LOG
#include "ptms/romuluslog/RomulusLog.hpp"
#define PTM_UPDATE_TX      romuluslog::RomulusLog::write_transaction
#define PTM_READ_TX        romuluslog::RomulusLog::read_transaction
#define PTM_ALLOC          romuluslog::RomulusLog::alloc
#define PTM_FREE           romuluslog::RomulusLog::free
#define PTM_NEW            romuluslog::RomulusLog::alloc
#define PTM_DELETE         romuluslog::RomulusLog::free
#define PTM_GET_ROOT       romuluslog::RomulusLog::get_object
#define PTM_PUT_ROOT       romuluslog::RomulusLog::put_object
#define TM_ALLOC           romuluslog::RomulusLog::alloc
#define TM_FREE            romuluslog::RomulusLog::free
#define TM_PMALLOC         romuluslog::RomulusLog::pmalloc
#define TM_PFREE           romuluslog::RomulusLog::pfree
#define TM_TYPE            romuluslog::persist
#define TM_NAME            romuluslog::RomulusLog::className
#define PTM_FLUSH          romuluslog::RomulusLog::log_flush_range

#elif defined USE_REDO
#define PTMDB_CAPTURE_BY_COPY
#include "ptms/redo/Redo.hpp"
#define PTM_UPDATE_TX      redo::Redo::updateTx
#define PTM_READ_TX        redo::Redo::readTx
#define PTM_ALLOC          redo::Redo::tmNew
#define PTM_FREE           redo::Redo::tmDelete
#define PTM_NEW            redo::Redo::tmNew
#define PTM_DELETE         redo::Redo::tmDelete
#define PTM_GET_ROOT       redo::Redo::get_object
#define PTM_PUT_ROOT       redo::Redo::put_object
#define TM_PMALLOC         redo::Redo::pmalloc
#define TM_PFREE           redo::Redo::pfree
#define TM_TYPE            redo::persist
#define TM_NAME            redo::Redo::className
#define PTM_LOG            redo::gRedo.dbLog
#define PTM_FLUSH          redo::gRedo.dbFlush

#elif defined USE_REDOTIMED
#define PTMDB_CAPTURE_BY_COPY
#include "ptms/redotimed/RedoTimed.hpp"
#define PTM_UPDATE_TX      redotimed::RedoTimed::updateTx
#define PTM_READ_TX        redotimed::RedoTimed::readTx
#define PTM_ALLOC          redotimed::RedoTimed::tmNew
#define PTM_FREE           redotimed::RedoTimed::tmDelete
#define PTM_NEW            redotimed::RedoTimed::tmNew
#define PTM_DELETE         redotimed::RedoTimed::tmDelete
#define PTM_GET_ROOT       redotimed::RedoTimed::get_object
#define PTM_PUT_ROOT       redotimed::RedoTimed::put_object
#define TM_PMALLOC         redotimed::RedoTimed::pmalloc
#define TM_PFREE           redotimed::RedoTimed::pfree
#define TM_TYPE            redotimed::persist
#define TM_NAME            redotimed::RedoTimed::className
#define PTM_LOG            redotimed::gRedo.dbLog
#define PTM_FLUSH          redotimed::gRedo.dbFlush

#elif defined USE_REDOOPT
#define PTMDB_CAPTURE_BY_COPY
#include "ptms/redoopt/RedoOpt.hpp"
#define PTM_UPDATE_TX      redoopt::RedoOpt::updateTx
#define PTM_READ_TX        redoopt::RedoOpt::readTx
#define PTM_ALLOC          redoopt::RedoOpt::tmNew
#define PTM_FREE           redoopt::RedoOpt::tmDelete
#define PTM_NEW            redoopt::RedoOpt::tmNew
#define PTM_DELETE         redoopt::RedoOpt::tmDelete
#define PTM_GET_ROOT       redoopt::RedoOpt::get_object
#define PTM_PUT_ROOT       redoopt::RedoOpt::put_object
#define TM_PMALLOC         redoopt::RedoOpt::pmalloc
#define TM_PFREE           redoopt::RedoOpt::pfree
#define TM_TYPE            redoopt::persist
#define TM_NAME            redoopt::RedoOpt::className
#define PTM_LOG            redoopt::gRedo.dbLog
#define PTM_FLUSH          redoopt::gRedo.dbFlush

#elif defined USE_OFWF
#define PTMDB_CAPTURE_BY_COPY
#include "ptms/ponefilewf/OneFilePTMWF.hpp"
#define PTM_UPDATE_TX      onefileptmwf::OneFileWF::updateTx
#define PTM_READ_TX        onefileptmwf::OneFileWF::readTx
#define PTM_ALLOC          onefileptmwf::OneFileWF::tmNew
#define PTM_FREE           onefileptmwf::OneFileWF::tmDelete
#define PTM_NEW            onefileptmwf::OneFileWF::tmNew
#define PTM_DELETE         onefileptmwf::OneFileWF::tmDelete
#define PTM_GET_ROOT       onefileptmwf::OneFileWF::get_object
#define PTM_PUT_ROOT       onefileptmwf::OneFileWF::put_object
#define TM_PMALLOC         onefileptmwf::OneFileWF::pmalloc
#define TM_PFREE           onefileptmwf::OneFileWF::pfree
#define TM_TYPE            onefileptmwf::tmtype
#define TM_NAME            onefileptmwf::OneFileWF::className
#define PTM_LOG            onefileptmwf::gCX.dbLog
#define PTM_FLUSH          onefileptmwf::gCX.dbFlush


#elif ROMULUS_LR_PTM
#include "romuluslr/RomulusLR.hpp"
#define TM_WRITE_TRANSACTION   romuluslr::RomulusLR::write_transaction
#define TM_READ_TRANSACTION    romuluslr::RomulusLR::read_transaction
#define TM_BEGIN_TRANSACTION() romuluslr::gRomLR.begin_transaction()
#define TM_END_TRANSACTION()   romuluslr::gRomLR.end_transaction()
#define TM_ALLOC               romuluslr::RomulusLR::alloc
#define TM_FREE                romuluslr::RomulusLR::free
#define TM_PMALLOC             romuluslr::RomulusLR::pmalloc
#define TM_PFREE               romuluslr::RomulusLR::pfree
#define TM_TYPE                romuluslr::persist
#define TM_NAME                romuluslr::RomulusLR::className
#define TM_CONSISTENCY_CHECK   romuluslr::RomulusLR::consistency_check
#define TM_INIT                romuluslr::RomulusLR::init

#elif PMDK_PTM
#include "pmdk/PMDKTM.hpp"
#define TM_WRITE_TRANSACTION   pmdk::PMDKTM::write_transaction
#define TM_READ_TRANSACTION    pmdk::PMDKTM::read_transaction
#define TM_ALLOC               pmdk::PMDKTM::alloc
#define TM_FREE                pmdk::PMDKTM::free
#define TM_PMALLOC             pmdk::PMDKTM::pmalloc
#define TM_PFREE               pmdk::PMDKTM::pfree
#define TM_TYPE                pmdk::persist
#define TM_NAME                pmdk::PMDKTM::className

#else
#error "PTM macros were undefined. You need a -DUSE_SOMETHING in the Makefile"
#endif



#endif  // _PTM_DB_INCLUDE_H_
