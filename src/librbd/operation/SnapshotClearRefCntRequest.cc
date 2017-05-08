// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "librbd/operation/SnapshotClearRefCntRequest.h"
#include "common/dout.h"
#include "common/errno.h"
#include "librbd/ImageCtx.h"

#define dout_subsys ceph_subsys_rbd
#undef dout_prefix
#define dout_prefix *_dout << "librbd::SnapshotClearRefCntRequest: "

namespace librbd {
namespace operation {

namespace {

template <typename I>
std::ostream& operator<<(std::ostream& os,
                         const typename SnapshotClearRefCntRequest<I>::State& state) {
  switch(state) {
  case SnapshotClearRefCntRequest<I>::STATE_CLEAR_SNAP_REFCNT:
    os << "CLEAR_SNAP_REFCNT";
    break;
  }
  return os;
}

} // anonymous namespace

template <typename I>
SnapshotClearRefCntRequest<I>::SnapshotClearRefCntRequest(I &image_ctx,
                          Context *on_finish,
						  const cls::rbd::SnapshotNamespace &snap_namespace,
						  const std::string &snap_name)
  : Request<I>(image_ctx, on_finish), m_snap_namespace(snap_namespace), m_snap_name(snap_name) {
}

template <typename I>
void SnapshotClearRefCntRequest<I>::send_op() {
  send_clear_snap_refcnt();
}

template <typename I>
bool SnapshotClearRefCntRequest<I>::should_complete(int r) {
  I &image_ctx = this->m_image_ctx;
  CephContext *cct = image_ctx.cct;
  ldout(cct, 5) << this << " " << __func__ << ": state=" << m_state << ", "
                << "r=" << r << dendl;
  if (r < 0) {
      lderr(cct) << "encountered error: " << cpp_strerror(r) << dendl;
  }
  return true;
}

template <typename I>
void SnapshotClearRefCntRequest<I>::send_clear_snap_refcnt() {
  I &image_ctx = this->m_image_ctx;
  assert(image_ctx.owner_lock.is_locked());

  CephContext *cct = image_ctx.cct;
  ldout(cct, 5) << this << " " << __func__ << dendl;

  m_state = STATE_CLEAR_SNAP_REFCNT;

  int r = verify_and_send_clear_snap_refcnt();
  if (r < 0) {
    this->async_complete(r);
    return;
  }
}

template <typename I>
int SnapshotClearRefCntRequest<I>::verify_and_send_clear_snap_refcnt() {
  I &image_ctx = this->m_image_ctx;
  RWLock::RLocker md_locker(image_ctx.md_lock);
  RWLock::RLocker snap_locker(image_ctx.snap_lock);

  CephContext *cct = image_ctx.cct;
  if ((image_ctx.features & RBD_FEATURE_LAYERING) == 0) {
    lderr(cct) << "image must support layering" << dendl;
    return -ENOSYS;
  }

  uint64_t snap_id = image_ctx.get_snap_id(m_snap_namespace, m_snap_name);
  if (snap_id == CEPH_NOSNAP) {
    return -ENOENT;
  }

  librados::ObjectWriteOperation op;
  cls_client::clear_snapshot_refcnt(&op, snap_id);

  librados::AioCompletion *rados_completion =
    this->create_callback_completion();
  int r = image_ctx.md_ctx.aio_operate(image_ctx.header_oid, rados_completion,
                                     &op);
  assert(r == 0);
  rados_completion->release();
  return 0;
}

} // namespace operation
} // namespace librbd

template class librbd::operation::SnapshotClearRefCntRequest<librbd::ImageCtx>;
