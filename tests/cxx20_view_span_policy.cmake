if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

foreach(_view_span_file IN ITEMS
  include/lvr2/types/Channel.hpp
  include/lvr2/types/Channel.tcc
  include/lvr2/types/BaseBuffer.hpp
  include/lvr2/types/BaseBuffer.tcc
  include/lvr2/types/VariantChannelMap.hpp
  include/lvr2/types/VariantChannelMap.tcc
)
  file(READ "${LVR2_SOURCE_DIR}/${_view_span_file}" _view_span_text)
  foreach(_banned_member_pattern IN ITEMS
    "std::string_view[ \t]+m_[A-Za-z0-9_]*[ \t;=]"
    "std::span[ \t]*<[^>]+>[ \t]+m_[A-Za-z0-9_]*[ \t;=]"
  )
    if(_view_span_text MATCHES "${_banned_member_pattern}")
      message(FATAL_ERROR "Non-owning view member is not allowed without an explicit owner contract in ${_view_span_file}: ${_banned_member_pattern}")
    endif()
  endforeach()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/types/Channel.hpp" _channel_header)
foreach(_required_channel_token IN ITEMS
  "#include <span>"
  "Channel(size_t n, size_t width, std::span<const T> values)"
  "std::span<const T> values() const noexcept"
  "std::span<T>       values() noexcept"
  "assign(std::span<const T> values)"
)
  string(FIND "${_channel_header}" "${_required_channel_token}" _required_channel_pos)
  if(_required_channel_pos EQUAL -1)
    message(FATAL_ERROR "Channel span API is missing required token: ${_required_channel_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/types/BaseBuffer.hpp" _base_buffer_header)
foreach(_required_buffer_token IN ITEMS
  "std::string_view name"
  "std::span<const T> values, std::string_view name"
  "addFloatChannel(std::span<const float> values"
  "addIndexChannel(std::span<const unsigned int> values"
  "addUCharChannel(std::span<const unsigned char> values"
)
  string(FIND "${_base_buffer_header}" "${_required_buffer_token}" _required_buffer_pos)
  if(_required_buffer_pos EQUAL -1)
    message(FATAL_ERROR "BaseBuffer view/span API is missing required token: ${_required_buffer_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/types/VariantChannelMap.hpp" _variant_channel_map_header)
foreach(_required_map_token IN ITEMS
  "struct StringKeyHash"
  "std::unordered_map<std::string, VariantChannel<T...>, StringKeyHash, std::equal_to<> >"
  "void add(std::string_view name"
  "Channel<U>& get(std::string_view name)"
  "getOptional(std::string_view name)"
)
  string(FIND "${_variant_channel_map_header}" "${_required_map_token}" _required_map_pos)
  if(_required_map_pos EQUAL -1)
    message(FATAL_ERROR "VariantChannelMap string_view API is missing required token: ${_required_map_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/migration_guide.md" _migration_guide)
foreach(_required_migration_token IN ITEMS
  "std::span"
  "std::string_view"
  "Channel<T>"
  "BaseBuffer"
)
  string(FIND "${_migration_guide}" "${_required_migration_token}" _required_migration_pos)
  if(_required_migration_pos EQUAL -1)
    message(FATAL_ERROR "Migration guide must document C++20 view/span API changes: ${_required_migration_token}")
  endif()
endforeach()

message(STATUS "C++20 view/span policy guard passed")
