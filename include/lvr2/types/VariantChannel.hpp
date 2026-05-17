#pragma once

#ifndef LVR2_TYPES_VARIANTCHANNEL
#define LVR2_TYPES_VARIANTCHANNEL

#include <variant>
#include <tuple>
#include <memory>
#include <optional>
#include <stdexcept>
#include <iostream>

#include "Channel.hpp"

namespace lvr2 {

template<typename... T>
class VariantChannel : public std::variant<Channel<T>...>
{
    using base = std::variant<Channel<T>...>;
protected:
    template <class T1, class Tuple>
    struct TupleIndex;

public:
    using base::base;
    using types = std::tuple<T...>;

    std::size_t which() const noexcept { return this->index(); }

    /**
     * @brief Access type index with type
     * - example: ChanneVariantMap<int, float> my_map;
     *            ChanneVariantMap<int, float>::type_index<int>::value -> 0
     */
    template<class U>
    struct index_of_type {
        static constexpr std::size_t value = TupleIndex<U, types>::value;
    };

    static constexpr std::size_t num_types = std::tuple_size<types>::value;

    template <std::size_t N>
    using type_of_index = typename std::tuple_element<N, types>::type;

    size_t numElements() const;

    size_t width() const;

    std::string typeName() const;

    template<typename U>
    std::shared_ptr<U[]> dataPtr() const;

    template<std::size_t N>
    std::shared_ptr<type_of_index<N>[]> dataPtr() const
    {
        return std::visit(DataPtrVisitor<type_of_index<N> >(), static_cast<const base&>(*this));
    }

    /**
     * @brief Get type index of a map entry
     * 
     */
    int type() const;

    // constexpr std::string typeString() const;


    template<typename U>
    Channel<U> extract() const;

    template<typename U>
    Channel<U>& extract();

    /**
     * @brief Checks if key has specific type U.
     * @example cm.is_type<float>() -> true
     */
    template<typename U>
    bool is_type() const;

    friend std::ostream& operator<<(std::ostream& os, const VariantChannel<T...>& ch)
    {
        os << "type: " << ch.typeName() << ", " << static_cast <const base &>(ch);
        return os;
    }

    VariantChannel<T...> clone() const;
    
// Visitor Implementations
protected:
    struct NumElementsVisitor
    {
        template<typename U>
        size_t operator()(const Channel<U>& channel) const
        {
            return channel.numElements();
        }
    };

    struct WidthVisitor
    {
        template<typename U>
        size_t operator()(const Channel<U>& channel) const
        {
            return channel.width();
        }
    };

    struct TypeNameVisitor
    {
        template<typename U>
        std::string operator()(const Channel<U>& channel) const
        {
            return channel.typeName();
        }
    };

    template<typename U>
    struct DataPtrVisitor
    {
        template<typename V>
        requires TypedChannel<Channel<V>, U>
        std::shared_ptr<U[]> operator()(const Channel<V>& channel) const
        {
            return channel.dataPtr();
        }

        template<typename V>
        requires (!TypedChannel<Channel<V>, U>)
        std::shared_ptr<U[]> operator()(const Channel<V>& channel) const
        {
            throw std::invalid_argument("tried to get wrong type of channel");
            return std::shared_ptr<U[]>();
        }
    };

    struct CloneVisitor
    {
        template<typename U>
        VariantChannel<T...> operator()(const Channel<U>& channel) const
        {
            return channel.clone();
        }
    };

    template <class T1, class... Types>
    struct TupleIndex<T1, std::tuple<T1, Types...>> {
        static constexpr std::size_t value = 0;
    };

    template <class T1, class U, class... Types>
    struct TupleIndex<T1, std::tuple<U, Types...>> {
        static constexpr std::size_t value = 1 + TupleIndex<T1, std::tuple<Types...>>::value;
    };

};

template<typename ...Tp>
using VariantChannelOptional = std::optional<VariantChannel<Tp...> >;


} // namespace lvr2

#include "VariantChannel.tcc"

#endif // LVR2_TYPES_VARIANTCHANNEL
