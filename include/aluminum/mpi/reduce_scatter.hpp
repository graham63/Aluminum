////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018, Lawrence Livermore National Security, LLC.  Produced at the
// Lawrence Livermore National Laboratory in collaboration with University of
// Illinois Urbana-Champaign.
//
// Written by the LBANN Research Team (N. Dryden, N. Maruyama, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-756777.
// All rights reserved.
//
// This file is part of Aluminum GPU-aware Communication Library. For details, see
// http://software.llnl.gov/Aluminum or https://github.com/LLNL/Aluminum.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "progress.hpp"
#include "mpi/base_state.hpp"
#include "mpi/communicator.hpp"
#include "mpi/utils.hpp"

namespace Al {
namespace internal {
namespace mpi {

template <typename T>
void passthrough_reduce_scatter(const T* sendbuf, T* recvbuf, size_t count,
                                ReductionOperator op, MPICommunicator& comm) {
  MPI_Reduce_scatter_block(buf_or_inplace(sendbuf), recvbuf, count,
                           TypeMap<T>(), ReductionOperator2MPI_Op(op),
                           comm.get_comm());
}

template <typename T>
class ReduceScatterAlState : public MPIState {
public:
  ReduceScatterAlState(const T* sendbuf_, T* recvbuf_, size_t count_,
                       ReductionOperator op_, MPICommunicator& comm_,
                       AlRequest req_) :
    MPIState(req_),
    sendbuf(sendbuf_), recvbuf(recvbuf_), count(count_),
    op(ReductionOperator2MPI_Op(op_)),
    comm(comm_.get_comm()) {}

  ~ReduceScatterAlState() override {}

  std::string get_name() const override { return "MPIReduceScatter"; }

protected:
  void start_mpi_op() override {
    MPI_Ireduce_scatter_block(buf_or_inplace(sendbuf), recvbuf, count,
                              TypeMap<T>(), op, comm, get_mpi_req());
  }

private:
  const T* sendbuf;
  T* recvbuf;
  size_t count;
  MPI_Op op;
  MPI_Comm comm;
};

template <typename T>
void passthrough_nb_reduce_scatter(const T* sendbuf, T* recvbuf, size_t count,
                                   ReductionOperator op, MPICommunicator& comm,
                                   AlRequest& req) {
  req = internal::get_free_request();
  internal::mpi::ReduceScatterAlState<T>* state =
    new internal::mpi::ReduceScatterAlState<T>(
      sendbuf, recvbuf, count, op, comm, req);
  get_progress_engine()->enqueue(state);
}

}  // namespace mpi
}  // namespace internal
}  // namespace Al
