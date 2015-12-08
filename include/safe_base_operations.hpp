#ifndef BOOST_NUMERIC_SAFE_BASE_OPERATIONS_HPP
#define BOOST_NUMERIC_SAFE_BASE_OPERATIONS_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include <type_traits> // is_base_of, is_same, enable_if
#include <ostream>
#include <istream>

#include <boost/config.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/utility/enable_if.hpp>

#include "safe_base.hpp"
#include "safe_compare.hpp"
#include "checked_result.hpp"
#include "interval.hpp"

namespace boost {
namespace numeric {

/////////////////////////////////////////////////////////////////
// validation

template<class Stored, Stored Min, Stored Max, class P, class E>
template<class T>
constexpr bool safe_base<Stored, Min, Max, P, E>::
validate(const T & t) const {
    static_assert(! is_safe<T>::value, "catch dumb mistake");
    // INT08-C
    return (
        ! safe_compare::less_than(
            Max,
            base_value(t)
        )
        &&
        ! safe_compare::less_than(
            base_value(t),
            Min
        )
    );
}

/////////////////////////////////////////////////////////////////
// construction and assignment operators

template<class Stored, Stored Min, Stored Max, class P, class E>
template<class T, T MinT, T MaxT, class PT, class ET>
constexpr safe_base<Stored, Min, Max, P, E>::
safe_base(const safe_base<T, MinT, MaxT, PT, ET> & t){
    constexpr const interval<T> t_interval = {
        MinT,
        MaxT
    };
    constexpr const interval<Stored> this_interval = {
        Min,
        Max
    };
    // if static values don't overlap, the program can never function
    static_assert(
        indeterminate(t_interval < this_interval),
        "safe type cannot be constructed with this type"
    );

    // if the argument interval is not contained in this interval
    if(! this_interval.includes(t_interval))
        // we have to validate the value we're initializing with
        if(! validate(t.m_t))
            E::range_error("Value out of range for this safe type");

    m_t = t.m_t;
}

template<class Stored, Stored Min, Stored Max, class P, class E>
template<class T, T MinT, T MaxT, class PT, class ET>
constexpr safe_base<Stored, Min, Max, P, E> &
safe_base<Stored, Min, Max, P, E>::
operator=(const safe_base<T, MinT, MaxT, PT, ET> & rhs){
    constexpr const interval<T> t_interval = {
        MinT,
        MaxT
    };
    // typedef print<decltype(t_interval)> pt_interval;
    constexpr const interval<Stored> this_interval = {
        Min,
        Max
    };
    // typedef print<decltype(this_interval)> pthis_intervall;

    /*
    typedef print<
        std::integral_constant<
            bool, (bool)(t_interval > this_interval)
        >
    > p_t_gt_this;

    typedef print<
        std::integral_constant<
            bool, (bool)(t_interval < this_interval)
        >
    > p_t_lt_this;
    */

    // if static values don't overlap, the program can never function
    static_assert(
        indeterminate(t_interval < this_interval),
        "safe type cannot be constructed with this type"
    );

    // if the argument interval is not contained in this interval
    if(! this_interval.includes(t_interval))
        // we have to validate the value we're assigning
        if(! validate(rhs.m_t))
            E::range_error("Value out of range for this safe type");

    m_t = rhs.m_t;
    return *this;
}

/////////////////////////////////////////////////////////////////
// casting operators
template< class Stored, Stored Min, Stored Max, class P, class E>
template<
    class R,
    typename std::enable_if<
        !boost::numeric::is_safe<R>::value,
        int
    >::type
>
constexpr safe_base<Stored, Min, Max, P, E>::
operator R () const {
    checked_result<R> r = checked::cast<R>(m_t);
    if(! r.no_exception())
        E::range_error(r);

    return static_cast<R>(r);
}

/////////////////////////////////////////////////////////////////
// binary operators

template<class T, class U>
struct common_policies {
    static_assert(
        boost::mpl::or_<
            is_safe<T>,
            is_safe<U>
        >::value,
        "at least one type must be a safe type"
    );

    static_assert(
        boost::mpl::or_<
            std::is_same<
                typename get_promotion_policy<T>::type,
                typename get_promotion_policy<U>::type
            >,
            std::is_same<
                typename get_promotion_policy<T>::type,
                void
            >,
            std::is_same<
                void,
                typename get_promotion_policy<U>::type
            >
        >::value,
        "if the promotion policies are different, one must be void!"
    );

    static_assert(
        boost::mpl::or_<
            std::is_same<
                typename get_exception_policy<T>::type,
                typename get_exception_policy<U>::type
            >,
            std::is_same<
                typename get_exception_policy<T>::type,
                void
            >,
            std::is_same<
                void,
                typename get_exception_policy<U>::type
            >
        >::value,
        "if the exception policies are different, one must be void!"
    );

    // now we've verified that there is no conflict between policies
    // return the one from the first safe type
    typedef
        typename boost::mpl::if_<
            is_safe<T>,
            T,
        typename boost::mpl::if_<
            is_safe<U>,
            U,
        //
            void
        >::type >::type safe_type;

    // typedef print<safe_type> p_st;
    static_assert(
        is_safe<safe_type>::value,
        "is_safe<safe_type>::value"
    );
/*
    static_assert(
        is_safe<U>::value,
        "is_safe<U>::value"
    );
    typedef print<safe_type> p_st;
*/
    static_assert(
        ! std::is_same<void, safe_type>::value,
        "asdfafs"
    );

    typedef typename get_promotion_policy<safe_type>::type promotion_policy;
    typedef typename get_exception_policy<safe_type>::type exception_policy;
};

// Note: the following global operators will be only found via
// argument dependent lookup.  So they won't conflict any
// other global operators for types in namespaces other than
// boost::numeric

/////////////////////////////////////////////////////////////////
// addition

template<class T, class U>
struct addition_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template addition_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    addition_result<T, U>
>::type
constexpr inline operator+(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef addition_result<T, U> ar;
    typedef typename ar::P::exception_policy exception_policy;
    typedef typename ar::type result_type;
    //typedef print<result_type> p_result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    typedef typename base_type<result_type>::type result_base_type;
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    // note: subtle trickery.  Suppose t is safe_range<MIN, ..>.  Then
    // std::numeric_limits<T>::min() will be safe_range<MIN with a value of MIN
    // Use base_value(T) ( which equals MIN ) to create a new interval. Same
    // for MAX.  Now
    constexpr const interval<t_base_type> t_interval(
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        );

    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    // when we add the temporary intervals above, we'll get a new interval
    // with the correct range for the sum !
    constexpr const checked_result<interval<result_base_type>> r_interval
        = add<result_base_type>(t_interval, u_interval);

    // if no over/under flow possible
    if(r_interval.no_exception()){
        return result_type(
            static_cast<result_base_type>(base_value(t))
            + static_cast<result_base_type>(base_value(u))
        );
    }

    // otherwise do the addition checking for overflow
    checked_result<result_base_type> r = checked::add<result_base_type>(
        base_value(t),
        base_value(u)
    );

    r.template dispatch<exception_policy>();

    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// subtraction
template<class T, class U>
struct subtraction_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template subtraction_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    subtraction_result<T, U>
>::type
constexpr operator-(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef subtraction_result<T, U> sr;
    typedef typename sr::P::exception_policy exception_policy;
    typedef typename sr::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    typedef typename base_type<result_type>::type result_base_type;
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    constexpr const checked_result<interval<result_base_type>> r_interval
        = subtract<result_base_type>(t_interval, u_interval);

    // if no over/under flow possible
    if(r_interval.no_exception()){
        return result_type(
            static_cast<result_base_type>(base_value(t))
            - static_cast<result_base_type>(base_value(u))
        );
    }

    // otherwise do the subtraction checking for overflow
    checked_result<result_base_type> r = checked::subtract<result_base_type>(
        base_value(t),
        base_value(u)
    );

    r.template dispatch<exception_policy>();
    
    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// multiplication

template<class T, class U>
struct multiplication_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template multiplication_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    multiplication_result<T, U>
>::type
constexpr operator*(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef multiplication_result<T, U> mr;
    typedef typename mr::P::exception_policy exception_policy;
    typedef typename mr::type result_type;
    // typedef print<result_type> p_result_type;

    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    typedef typename base_type<result_type>::type result_base_type;
    // typedef print<result_base_type> p_result_base_type;
    typedef typename base_type<T>::type t_base_type;
    // typedef print<t_base_type> p_t_base_type;
    typedef typename base_type<U>::type u_base_type;
    // typedef print<u_base_type> p_u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    // when we multiply the temporary intervals above, we'll get a new interval
    // with the correct range for the  product!
    constexpr const checked_result<interval<result_base_type>> r_interval
        = multiply<result_base_type>(t_interval, u_interval);

    // if no over/under flow possible
    if(r_interval.no_exception())
        return result_type(
            static_cast<result_base_type>(base_value(t))
            * static_cast<result_base_type>(base_value(u))
        );

    // otherwise do the multiplication checking for overflow
    const checked_result<result_base_type>  r = checked::multiply<result_base_type>(
        base_value(t),
        base_value(u)
    );

    r.template dispatch<exception_policy>();

    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// division

template<class T, class U>
struct division_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template division_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    division_result<T, U>
>::type
constexpr operator/(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef division_result<T, U> dr;
    typedef typename dr::P::exception_policy exception_policy;
    typedef typename dr::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );
    typedef typename base_type<result_type>::type result_base_type;
    // typedef print<result_type> p_result_type;
    /*
    // note these assertions will trap when using native promotion
    // policy.  This is because things unsigned char get promoted 
    // to int when used in operations. comment them out as a reminder
    // not to assume that unsigned + unsigned -> unsigned
    static_assert(
        std::is_signed<result_type>::value ==
        std::is_signed<result_base_type>::value
        ,
        "base_type changed signedness"
    );
    // typedef print<result_base_type> p_result_base_type;
    static_assert(
        std::is_signed<T>::value != std::is_signed<U>::value
        ||
        std::is_signed<result_base_type>::value == std::is_signed<T>::value
        ,
        "unexpected type for result"
    );
    */

    typedef typename base_type<T>::type t_base_type;
    // typedef print<t_base_type> p_t_base_type;
    typedef typename base_type<U>::type u_base_type;
    // typedef print<u_base_type> p_u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    // when we divide the temporary intervals above, we'll get a new interval
    // with the correct range for the result!
    constexpr const checked_result<interval<result_base_type>> r_interval
        { divide<result_base_type>(t_interval, u_interval) };

    // if no over/under flow or domain error possible
    if(r_interval.no_exception()
    // and if the denominator cannot contain zero
    && (u_interval.l <= 0 && u_interval.u >=0 )
    )
        // we can just invoke the operation and return - even at compile
        // time since this is a constexpr !
        return result_type(
            static_cast<result_base_type>(base_value(t))
            / static_cast<result_base_type>(base_value(u))
        );

    // otherwise do the division checking for overflow
    typedef typename dr::P::promotion_policy promotion_policy;
    checked_result<result_base_type>  r =
        promotion_policy::template divide<result_base_type>(
            base_value(t),
            base_value(u)
        );
    r.template dispatch<exception_policy>();
    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// modulus

template<class T, class U>
struct modulus_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template modulus_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    modulus_result<T, U>
>::type
inline operator%(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef modulus_result<T, U> mr;
    typedef typename mr::P::exception_policy exception_policy;
    typedef typename mr::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    typedef typename base_type<result_type>::type result_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    // if denominator excludes zero
    if(u_interval.l > 0 || 0 > u_interval.u){
        // no checking necessary
        return result_type(base_value(t) % base_value(u));
    }

    using t_type = decltype(base_value(t) % base_value(u));

    // otherwise do the modulus checking for overflow
    typedef modulus_result<T, U> mr;
    typedef typename mr::P::promotion_policy promotion_policy;

    checked_result<t_type>  r =
        promotion_policy::template modulus<t_type>(
            base_value(t),
            base_value(u)
        );

    r.template dispatch<exception_policy>();
    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// comparison

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator<(const T & lhs, const U & rhs) {
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    constexpr const boost::logic::tribool r =
        t_interval < u_interval;

    return
        // if the ranges don't overlap
        (! boost::logic::indeterminate(r)) ?
            // values in those ranges can't be equal
            false
        :
            // otherwise we have to check
            safe_compare::less_than(base_value(lhs), base_value(rhs));
        ;
}

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator>(const T & lhs, const U & rhs) {
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    constexpr const boost::logic::tribool r =
        t_interval > u_interval;

    return
        // if the ranges don't overlap
        (! boost::logic::indeterminate(r)) ?
            // values in those ranges can't be equal
            false
        :
            // otherwise we have to check
            safe_compare::greater_than(base_value(lhs), base_value(rhs));
        ;
}

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator==(const T & lhs, const U & rhs) {
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    return
        // if the ranges don't overlap
        ( t_interval < u_interval || t_interval > u_interval) ?
            // values in those ranges can't be equal
            false
        :
            // otherwise we have to check
            safe_compare::equal(base_value(lhs), base_value(rhs));
        ;
}

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator!=(const T & lhs, const U & rhs) {
    return ! (lhs == rhs);
}

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator>=(const T & lhs, const U & rhs) {
    return ! ( rhs < lhs );
}

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    boost::mpl::identity<bool>
>::type
constexpr operator<=(const T & lhs, const U & rhs) {
    return ! ( rhs > lhs );
}

/////////////////////////////////////////////////////////////////
// shift operators

template<class T, class U>
struct left_shift_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template left_shift_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if_c<
    // handle safe<T> << int, int << safe<U>, safe<T> << safe<U>
    // exclude std::ostream << ...
    (! std::is_base_of<std::ios_base, T>::value)
    && (
        boost::numeric::is_safe<T>::value
        ||boost::numeric::is_safe<U>::value
    ),
    left_shift_result<T, U>
>::type
constexpr inline operator<<(const T & t, const U & u){
    // INT13-CPP
    typedef left_shift_result<T, U> lsr;
    typedef typename lsr::P::exception_policy exception_policy;
    typedef typename lsr::type result_type;
    typedef typename base_type<result_type>::type result_base_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    constexpr const checked_result<interval<result_base_type>> r_interval {
        left_shift<result_base_type>(t_interval, u_interval)
    };

    // if it's impossible to create an invalid result
    if(r_interval.no_exception())
        // just return the normal calcuation
        return result_type(t << u);

    const checked_result<result_base_type> r = checked::left_shift<result_base_type>(
        base_value(t),
        base_value(u)
    );

    r.template dispatch<exception_policy>();
    return result_type(static_cast<result_base_type>(r));
}

template<class T, class U>
struct right_shift_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template right_shift_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if_c<
    // handle safe<T> << int, int << safe<U>, safe<T> << safe<U>
    // exclude std::ostream << ...
    (! std::is_base_of<std::ios_base, T>::value)
    && (
        boost::numeric::is_safe<T>::value
        ||boost::numeric::is_safe<U>::value
    ),
    right_shift_result<T, U>
>::type
constexpr inline operator>>(const T & t, const U & u){
    // INT13-CPP
    typedef right_shift_result<T, U> rsr;
    typedef typename rsr::P::exception_policy exception_policy;
    typedef typename rsr::type result_type;
    typedef typename base_type<result_type>::type result_base_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );
    typedef typename base_type<T>::type t_base_type;
    typedef typename base_type<U>::type u_base_type;

    // filter out case were overflow cannot occur
    constexpr const interval<t_base_type> t_interval = {
            base_value(std::numeric_limits<T>::min()),
            base_value(std::numeric_limits<T>::max())
        };
    constexpr const interval<u_base_type> u_interval = {
            base_value(std::numeric_limits<U>::min()),
            base_value(std::numeric_limits<U>::max())
        };

    constexpr const checked_result<interval<result_base_type>> r_interval {
        right_shift<result_base_type>(t_interval, u_interval)
    };

    // if it's impossible to create an invalid result
    if(r_interval.no_exception())
        // just return the normal calcuation
        return result_type(t >> u);

    const checked_result<result_base_type> r = checked::right_shift<result_base_type>(
        base_value(t),
        base_value(u)
    );

    r.template dispatch<exception_policy>();
    return result_type(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// bitwise operators

// operator |
template<class T, class U>
struct or_result {
    typedef common_policies<T, U> P;
    typedef typename P::promotion_policy::template or_result<
        T,
        U,
        typename P::promotion_policy,
        typename P::exception_policy
    >::type type;
};

template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    or_result<T, U>
>::type
constexpr inline operator|(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef or_result<T, U> or_;
    typedef typename or_::P::exception_policy exception_policy;
    typedef typename or_::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    static_assert(
        std::numeric_limits<T>::is_integer
        && (! std::numeric_limits<T>::is_signed)
        && std::numeric_limits<U>::is_integer
        && (! std::numeric_limits<U>::is_signed),
        // INT13-C
        "bitwise operations are only applicable to unsigned integers"
    );

    typedef typename base_type<result_type>::type result_base_type;

    static_assert(
        std::numeric_limits<result_base_type>::is_integer
        && (! std::numeric_limits<result_base_type>::is_signed),
        // INT13-C
        "bitwise operations should return unsigned integers"
    );

    checked_result<result_base_type> r = checked::bitwise_or<result_base_type>(t, u);
    r.template dispatch<exception_policy>();
    return static_cast<result_type>(static_cast<result_base_type>(r));
}

// operator &
template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    or_result<T, U>
>::type
constexpr inline operator&(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef or_result<T, U> and_;
    typedef typename and_::P::exception_policy exception_policy;
    typedef typename and_::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    static_assert(
        std::numeric_limits<T>::is_integer
        && (! std::numeric_limits<T>::is_signed)
        && std::numeric_limits<U>::is_integer
        && (! std::numeric_limits<U>::is_signed),
        // INT13-C
        "bitwise operations are only applicable to unsigned integers"
    );

    typedef typename base_type<result_type>::type result_base_type;

    static_assert(
        std::numeric_limits<result_base_type>::is_integer
        && (! std::numeric_limits<result_base_type>::is_signed),
        // INT13-C
        "bitwise operations should return unsigned integers"
    );

    checked_result<result_base_type> r = checked::bitwise_and<result_base_type>(t, u);
    r.template dispatch<exception_policy>();
    return static_cast<result_type>(static_cast<result_base_type>(r));
}

// operator ^
template<class T, class U>
typename boost::lazy_enable_if<
    boost::mpl::or_<
        boost::numeric::is_safe<T>,
        boost::numeric::is_safe<U>
    >,
    or_result<T, U>
>::type
constexpr inline operator^(const T & t, const U & u){
    // argument dependent lookup should guarentee that we only get here
    // only if one of the types is a safe type. Verify this here
    typedef or_result<T, U> xor_;
    typedef typename xor_::P::exception_policy exception_policy;
    typedef typename xor_::type result_type;
    static_assert(
        boost::numeric::is_safe<result_type>::value,
        "Promotion failed to return safe type"
    );

    static_assert(
        std::numeric_limits<T>::is_integer
        && (! std::numeric_limits<T>::is_signed)
        && std::numeric_limits<U>::is_integer
        && (! std::numeric_limits<U>::is_signed),
        // INT13-C
        "bitwise operations are only applicable to unsigned integers"
    );

    typedef typename base_type<result_type>::type result_base_type;

    static_assert(
        std::numeric_limits<result_base_type>::is_integer
        && (! std::numeric_limits<result_base_type>::is_signed),
        // INT13-C
        "bitwise operations should return unsigned integers"
    );

    checked_result<result_base_type> r = checked::bitwise_xor<result_base_type>(t, u);
    r.template dispatch<exception_policy>();
    return static_cast<result_type>(static_cast<result_base_type>(r));
}

/////////////////////////////////////////////////////////////////
// stream operators

template<
    class T,
    T Min,
    T Max,
    class P, // promotion polic
    class E  // exception policy
>
std::ostream & operator<<(
    std::ostream & os,
    const safe_base<T, Min, Max, P, E> & t
){
    os << (
        (std::is_same<T, signed char>::value
        || std::is_same<T, unsigned char>::value
        ) ?
            static_cast<int>(t.m_t)
        :
            t.m_t
    );
    return os;
}

namespace detail {

    template<class T, class Enable = void>
    struct Temp {
        T value;
    }; // primary template
     
    template<class T>
    struct Temp<
        T,
        typename std::enable_if<
            (std::is_same<T, signed char>::value
            || std::is_same<T, unsigned char>::value)
        >
    >
    {
        int value;
    };

} // detail

template<
    class T,
    T Min,
    T Max,
    class P, // promotion polic
    class E  // exception policy
>
std::istream & operator>>(
    std::istream & is,
    safe_base<T, Min, Max, P, E> & t
){

    detail::Temp<T> tx;
    is >> tx.value;
    if(is.fail())
        E::range_error("error in file input");
    t = tx.value;
    return is;
}

} // numeric
} // boost

#endif // BOOST_NUMERIC_SAFE_BASE_OPERATIONS_HPP
