// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef CEPH_LIBRBD_OPERATION_SNAPSHOT_SUB_REFCNT_REQUEST_H
#define CEPH_LIBRBD_OPERATION_SNAPSHOT_SUB_REFCNT_REQUEST_H

#include "librbd/operation/Request.h"
#include <string>

class Context;

namespace librbd {

class ImageCtx;

namespace operation {

template <typename ImageCtxT = ImageCtx>
class SnapshotSubRefCntRequest : public Request<ImageCtxT> {
public:
  /**
   * Snap Protect goes through the following state machine:
   *
   * @verbatim
   *
   * <start>
   *    |
   *    v
   * STATE_SUB_SNAP_REFCNT
   *    |
   *    v
   * <finish>
   *
   * @endverbatim
   *
   */
  enum State {
    STATE_SUB_SNAP_REFCNT
  };

  SnapshotSubRefCntRequest(ImageCtxT &image_ctx, Context *on_finish,
             const cls::rbd::SnapshotNamespace &snap_namespace,
             const std::string &snap_name);

protected:
  virtual void send_op();
  virtual bool should_complete(int r);

  virtual journal::Event create_event(uint64_t op_tid) const {
    return journal::SnapSubRefCntEvent(op_tid, m_snap_namespace, m_snap_name);
  }

private:
  cls::rbd::SnapshotNamespace m_snap_namespace;
  std::string m_snap_name;
  State m_state;

  void send_sub_snap_refcnt();

  int verify_and_send_sub_snap_refcnt();
};

} // namespace operation
} // namespace librbd

extern template class librbd::operation::SnapshotSubRefCntRequest<librbd::ImageCtx>;

#endif // CEPH_LIBRBD_OPERATION_SNAPSHOT_SUB_REFCNT_REQUEST_H
