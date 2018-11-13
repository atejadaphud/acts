// This file is part of the Acts project.
//
// Copyright (C) 2018 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Utilities/Definitions.hpp"

namespace Acts {
namespace detail {
  /// The dummy list call implementation
  template <unsigned int N>
  struct stepper_extension_list_impl;

  /// The extension list call implementation
  /// - it calls 'validExtensionsForStep()' on the current entry of the tuple
  /// - it stores the result in @p validExtensions
  /// - then broadcasts the extension call to the remaining tuple
  template <unsigned int N>
  struct stepper_extension_list_impl
  {
    template <typename stepper_state_t, typename... T>
    static void
    validExtensionForStep(std::tuple<T...>&      obs_tuple,
                          const stepper_state_t& state,
                          std::array<int, sizeof...(T)>& validExtensions)
    {
      validExtensions.at(N - 1)
          = std::get<N - 1>(obs_tuple).validExtensionForStep(state);
      stepper_extension_list_impl<N - 1>::validExtensionForStep(
          obs_tuple, state, validExtensions);
    }

    /// The extension list call implementation
    /// - it calls 'k()' on the current entry of the tuple
    /// - then broadcasts the extension call to the remaining tuple
    template <typename stepper_state_t, typename... T>
    static bool
    k(std::tuple<T...>&      obs_tuple,
      const stepper_state_t& state,
      Vector3D&              knew,
      const Vector3D&        bField,
      const std::array<bool, sizeof...(T)>& validExtensions,
      const int       i     = 0,
      const double    h     = 0,
      const Vector3D& kprev = Vector3D())
    {
      // If element is invalid: continue
      if (!validExtensions.at(N - 1)) {
        return stepper_extension_list_impl<N - 1>::k(
            obs_tuple, state, knew, bField, validExtensions, i, h, kprev);
      }

      // Continue as long as evaluations are 'true'
      if (std::get<N - 1>(obs_tuple).k(state, knew, bField, i, h, kprev)) {
        return stepper_extension_list_impl<N - 1>::k(
            obs_tuple, state, knew, bField, validExtensions, i, h, kprev);
      } else {
        // Break at false
        return false;
      }
    }

    /// The extension list call implementation
    /// - it calls 'finalize()' on the current entry of the tuple
    /// - then broadcasts the extension call to the remaining tuple
    template <typename stepper_state_t, typename stepper_data_t, typename... T>
    static bool
    finalize(const std::tuple<T...>& obs_tuple,
             stepper_state_t&        state,
             const double            h,
             const stepper_data_t&   data,
             ActsMatrixD<7, 7>&                    D,
             const std::array<bool, sizeof...(T)>& validExtensions)
    {
      // If element is invalid: continue
      if (!validExtensions.at(N - 1)) {
        return stepper_extension_list_impl<N - 1>::finalize(
            obs_tuple, state, h, data, D, validExtensions);
      }

      // Continue as long as evaluations are 'true'
      if (std::get<N - 1>(obs_tuple).finalize(state, h, data, D)) {
        return stepper_extension_list_impl<N - 1>::finalize(
            obs_tuple, state, h, data, D, validExtensions);
      } else {
        // Break at false
        return false;
      }
    }

    /// The extension list call implementation
    /// - it calls 'finalize()' on the current entry of the tuple
    /// - then broadcasts the extension call to the remaining tuple
    template <typename stepper_state_t, typename... T>
    static bool
    finalize(const std::tuple<T...>& obs_tuple,
             stepper_state_t&        state,
             const double            h,
             const std::array<bool, sizeof...(T)>& validExtensions)
    {
      // If element is invalid: continue
      if (!validExtensions.at(N - 1)) {
        return stepper_extension_list_impl<N - 1>::finalize(
            obs_tuple, state, h, validExtensions);
      }

      // Continue as long as evaluations are 'true'
      if (std::get<N - 1>(obs_tuple).finalize(state, h)) {
        return stepper_extension_list_impl<N - 1>::finalize(
            obs_tuple, state, h, validExtensions);
      } else {
        // Break at false
        return false;
      }
    }
  };

  template <>
  struct stepper_extension_list_impl<0u>
  {
    /// The extension list call implementation
    /// - it calls 'validExtensionsForStep()' on the last entry of the tuple
    /// - it stores the result in @p validExtensions
    template <typename stepper_state_t, typename... T>
    static void
    validExtensionForStep(const std::tuple<T...>& obs_tuple,
                          const stepper_state_t&  state,
                          std::array<int, sizeof...(T)>& validExtensions)
    {
      validExtensions.at(0)
          = std::get<0>(obs_tuple).validExtensionForStep(state);
    }

    /// The extension list call implementation
    /// - it calls 'k()' on the last entry of the tuple
    template <typename stepper_state_t, typename... T>
    static bool
    k(std::tuple<T...>&      obs_tuple,
      const stepper_state_t& state,
      Vector3D&              knew,
      const Vector3D&        bField,
      const std::array<bool, sizeof...(T)>& validExtensions,
      const int       i     = 0,
      const double    h     = 0,
      const Vector3D& kprev = Vector3D())
    {
      if (!validExtensions.at(0)) {
        return true;
      }

      return std::get<0>(obs_tuple).k(state, knew, bField, i, h, kprev);
    }

    /// The extension list call implementation
    /// - it calls 'finalize()' on the last entry of the tuple
    template <typename stepper_state_t, typename stepper_data_t, typename... T>
    static bool
    finalize(const std::tuple<T...>& obs_tuple,
             stepper_state_t&        state,
             const double            h,
             const stepper_data_t&   data,
             ActsMatrixD<7, 7>&                    D,
             const std::array<bool, sizeof...(T)>& validExtensions)
    {
      if (!validExtensions.at(0)) {
        return true;
      }

      return std::get<0>(obs_tuple).finalize(state, h, data, D);
    }

    /// The extension list call implementation
    /// - it calls 'finalize()' on the last entry of the tuple
    template <typename stepper_state_t, typename... T>
    static bool
    finalize(const std::tuple<T...>& obs_tuple,
             stepper_state_t&        state,
             const double            h,
             const std::array<bool, sizeof...(T)>& validExtensions)
    {
      if (!validExtensions.at(0)) {
        return true;
      }

      return std::get<0>(obs_tuple).finalize(state, h);
    }
  };

  //~ /// The empty extension list call implementation
  //~ template <>
  //~ struct stepper_extension_list_impl<>
  //~ {

  //~ /// The empty extension list call implementation
  //~ template <typename stepper_state_t, typename... T>
  //~ static void
  //~ validExtensionForStep(const std::tuple<T...>& /*unused*/,
  //~ const stepper_state_t& /*unused*/,
  //~ std::array<int, sizeof...(T)>& /*unused*/)
  //~ {
  //~ }

  //~ /// The empty extension list call implementation
  //~ template <typename stepper_state_t, typename... T>
  //~ static bool
  //~ k(std::tuple<T...>& /*unused*/,
  //~ const stepper_state_t& /*unused*/,
  //~ Vector3D& /*unused*/,
  //~ const Vector3D& /*unused*/,
  //~ std::array<bool, sizeof...(T)>& /*unused*/,
  //~ const int /*unused*/,
  //~ const double /*unused*/,
  //~ const Vector3D& /*unused*/)
  //~ {
  //~ return true;
  //~ }

  //~ /// The empty extension list call implementation
  //~ template <typename stepper_state_t, typename stepper_data_t, typename...
  // T>
  //~ static bool
  //~ finalize(const std::tuple<T...>& /*unused*/,
  //~ stepper_state_t& /*unused*/,
  //~ const double /*unused*/,
  //~ const stepper_data_t& /*unused*/,
  //~ ActsMatrixD<7, 7>& /*unused*/,
  //~ std::array<bool, sizeof...(T)>& /*unused*/)
  //~ {
  //~ return true;
  //~ }

  //~ /// The empty extension list call implementation
  //~ template <typename stepper_state_t, typename... T>
  //~ static bool
  //~ finalize(const std::tuple<T...>& /*unused*/,
  //~ stepper_state_t& /*unused*/,
  //~ const double /*unused*/,
  //~ std::array<bool, sizeof...(T)>& /*unused*/)
  //~ {
  //~ return true;
  //~ }
  //~ };

}  // namespace detail
}  // namespace Acts
