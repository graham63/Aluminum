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
#include "mpi/communicator.hpp"
#include "mpi/utils.hpp"

namespace Al {
namespace internal {
namespace mpi {

template <typename T>
void passthrough_allgather(const T* sendbuf, T* recvbuf, size_t count,
                           MPICommunicator& comm) {
  MPI_Allgather(buf_or_inplace(sendbuf), count, TypeMap<T>(),
                recvbuf, count, TypeMap<T>(), comm.get_comm());
}

template <typename T>
class AllgatherAlState : public AlState {
public:
  AllgatherAlState(const T* sendbuf_, T* recvbuf_, size_t count_,
                   MPICommunicator& comm_, AlRequest req_) :
    AlState(req_),
    sendbuf(sendbuf_), recvbuf(recvbuf_), count(count_),
    comm(comm_.get_comm()) {}

  ~AllgatherAlState() override {}

  void start() override {
    AlState::start();
    MPI_Iallgather(buf_or_inplace(sendbuf), count, TypeMap<T>(),
                   recvbuf, count, TypeMap<T>(), comm, &mpi_req);
  }

  PEAction step() override {
    int flag;
    MPI_Test(&mpi_req, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      return PEAction::complete;
    }
    return PEAction::cont;
  }

  std::string get_name() const override { return "MPIAllgather"; }

private:
  const T* sendbuf;
  T* recvbuf;
  size_t count;
  MPI_Comm comm;
  MPI_Request mpi_req;
};

template <typename T>
void passthrough_nb_allgather(const T* sendbuf, T* recvbuf, size_t count,
                              MPICommunicator& comm, AlRequest& req) {
  req = internal::get_free_request();
  internal::mpi::AllgatherAlState<T>* state =
    new internal::mpi::AllgatherAlState<T>(
      sendbuf, recvbuf, count, comm, req);
  get_progress_engine()->enqueue(state);
}

}  // namespace mpi
}  // namespace internal
}  // namespace Al