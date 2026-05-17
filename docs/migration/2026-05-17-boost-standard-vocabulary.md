# Boost standard-library replacement migration

## Affected users

Users who include LVR public headers that previously exposed Boost standard-equivalent types.

## Old behavior

Several public APIs exposed Boost vocabulary types such as `boost::optional`, `boost::variant`, `boost::shared_array`, `boost::shared_ptr`, and `boost::filesystem::path`. Consumers needed Boost headers even for standard C++20 use cases.

## New behavior

The touched public APIs now use C++20 standard vocabulary directly:

- paths use `std::filesystem::path`;
- optional values use `std::optional` and optional references use `std::optional<std::reference_wrapper<T>>`;
- variants use `std::variant`/`std::visit` behind the existing LVR `Variant` and `VariantChannel` wrappers;
- channel and legacy array owners use `std::shared_ptr<T[]>` with existing `std::span` views for non-owning access;
- mutex/timer helpers use `std::mutex`, `std::scoped_lock`, and `std::chrono`.

Hard Boost cases remain isolated for follow-up work: mapped files, binary archive serialization, property-tree XML parsing, MPI packaging, and stale tool link cleanup.

## Migration steps

- Replace Boost path aliases with `std::filesystem`.
- Replace `boost::optional<T>` with `std::optional<T>` and `boost::none` with `std::nullopt`.
- For optional references, use `opt->get()` or `opt.value().get()` to access the referenced object.
- Replace `boost::shared_array<T>` uses with `std::shared_ptr<T[]>` when interacting with unchanged LVR array-owner APIs, or prefer span/vector APIs where available.
- Replace `boost::variant` visitors with `std::visit` and normal callable visitors.

## Coverage

- `lvr2_boost_stdlib_replacements` policy guard bans reintroduction of standard-equivalent Boost headers, namespaces, and package metadata.
- `lvr2_cxx20_view_span_contracts` now checks `Channel<T>::DataPtr` ownership and optional-reference mutation behavior.

Related ADRs: ADR 0018 and ADR 0019.
