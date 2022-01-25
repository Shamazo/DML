/*
 * Copyright 2021 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 *
 */

#include <dml_dif.h>

#include <core/utils.hpp>
#include <cstring>
#include <dml/detail/common/status.hpp>
#include <dml/detail/common/utils/enum.hpp>

#include "../../../../include/dml/dmldefs.h"
#include "immintrin.h"
#include "kernels.hpp"

namespace dml::core::kernels
{
    void dif_check(const_view<descriptor, operation::dif_check> dsc) noexcept
    {
        auto record = make_view<operation::dif_check>(get_completion_record(dsc));

        const auto src              = reinterpret_cast<byte_t *>(dsc.source_address());
        const auto transfer_size    = dsc.transfer_size();
        const auto options          = dsc.flags();
        const auto dif_options      = dsc.dif_flags();
        const auto dif_src_options  = dsc.source_dif_flags();
        const auto src_app_tag_mask = dsc.source_app_tag_mask();

        const auto src_ref_tag = dsc.source_ref_tag();
        const auto src_app_tag = dsc.source_app_tag();

        dml_job_t job;
        memset(&job, 0, sizeof(dml_job_t));
        job.source_first_ptr                       = src;
        job.source_length                          = transfer_size;
        job.operation                              = DML_OP_DIF_CHECK;
        job.dif_config.source_reference_tag_seed   = src_ref_tag;
        job.dif_config.source_application_tag_seed = src_app_tag;
        job.dif_config.source_application_tag_mask = src_app_tag_mask;
        job.dif_config.block_size                  = static_cast<dml_dif_block_size_t>(dif_options & 0b11);

        // Job API composes DIF flags into one 64-bit value via shifting, check dmldefs.h
        job.dif_config.flags = (uint64_t(dif_options) << 16) | dif_src_options;

        job.flags = options;

        auto status = dml_legacy_dif_check(&job);

        record.dif_status()      = job.result;
        record.bytes_completed() = job.offset;
        // TODO: Tags should be written

        _mm_mfence();
        record.status() = to_underlying((status == DML_STATUS_OK) ? dml::detail::execution_status::success
                                                                  : dml::detail::execution_status::dif_control_error);
    }
}  // namespace dml::core::kernels
