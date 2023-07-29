#pragma once
#include <iostream>
#include <type_traits>
#include <vector>
namespace uapi {
namespace traits {

//////////////
/// type helpers
// inheritance
template <class C>
concept CanBeIterated = requires(C c) {
                          c.begin();
                          c.end();
                        } && !
std::is_same_v<std::decay_t<C>, std::string>;

// Vectors
template <class T> struct is_vector : std::false_type {};
template <class T, class A>
struct is_vector<std::vector<T, A>> : std::true_type {};

template <class T> struct is_array : std::false_type {};
template <class T, size_t A>
struct is_array<std::array<T, A>> : std::true_type {};

template <typename T>
concept Arr = is_array<std::decay_t<T>>::value;

template <typename T>
concept Vec = is_vector<std::decay_t<T>>::value;

template <typename T>
concept VecOrArr = Vec<T> || Arr<T>;

template <typename T> struct inner_type {
  using type = T;
};
template <typename T> struct inner_type<std::vector<T>> {
  using type = typename inner_type<T>::type;
};
template <typename T, size_t N> struct inner_type<std::array<T, N>> {
  using type = typename inner_type<T>::type;
};

// Tuples
template <typename T>
concept TupleLike = requires(T a) {
                      std::tuple_size<T>::value;
                      std::get<0>(a);
                    } && !
VecOrArr<T>;

// String
template <typename T>
concept Str = std::is_same_v<std::decay_t<T>, std::string>;

/////////////////
// type refs

template <typename T> struct unwrap_ref {
  using type = T;
};
template <typename T> struct unwrap_ref<std::reference_wrapper<T>> {
  using type = std::decay_t<T>;
};

/////////////////
// behaviours
template <typename T>
concept printable = requires(T t) {
                      // { std::cout << t } -> std::same_as<std::ostream &>;
                      std::cout << t;
                    };

} // namespace traits
} // namespace uapi
