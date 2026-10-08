#ifndef CMF_EVENTHANDLE_H
#define CMF_EVENTHANDLE_H

#include <concepts>
#include <functional>
#include <tuple>
#include <forward_list>
#include "Object/Object.h"
#include "Memory/SmartPtr/WeakObjectPtr.h"
#include "Containers/Queue.h"
#include "Util/stdafx.h"
#include "Log/Log.h"
#include "Statics/ApplicationStatics.h"
#include "EventScanner.h"
#include "Core/Application.h"

/**
 * @brief A static helper function used to simplify binding of object member functions to events as callbacks.
 * @tparam Args The types of arguments of the bound function.
 */
template<typename ...Args>
struct BindHelper {
	/**
	 * @brief Binding a function with more than 29 argument is not possible.
	 * This function variation ensures correct error logging in such case.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return Always nullptr.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept {
		CMF_LOG(CMF, Error, "Unsupported bind attempted on a function with more than 31 argument.");
		return nullptr;
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 0) {
		return std::bind(function, object);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 1) {
		return std::bind(function, object, std::placeholders::_1);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 2) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 3) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 4) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 5) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 6) {
		return std::bind(function, object, std::placeholders::_1,
									std::placeholders::_2,
									std::placeholders::_3,
									std::placeholders::_4,
									std::placeholders::_5,
									std::placeholders::_6);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 7) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 8) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 9) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 10) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 11) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 12) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 13) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 14) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 15) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 16) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 17) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 18) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 19) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 20) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 21) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 22) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 23) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 24) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 25) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24,
						 std::placeholders::_25);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 26) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24,
						 std::placeholders::_25,
						 std::placeholders::_26);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 27) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24,
						 std::placeholders::_25,
						 std::placeholders::_26,
						 std::placeholders::_27);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 28) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24,
						 std::placeholders::_25,
						 std::placeholders::_26,
						 std::placeholders::_27,
						 std::placeholders::_28);
	}

	/**
	 * @brief Function helper for binding a function with various number of arguments.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The object instance upon which the function will be bound and called.
	 * @param function The function being bound.
	 * @return The object member function in a std::function wrapper.
	 */
	template<typename O, typename F>
	inline static constexpr std::function<void(Args...)> get(O* object, F&& function) noexcept requires (sizeof...(Args) == 29) {
		return std::bind(function, object, std::placeholders::_1,
						 std::placeholders::_2,
						 std::placeholders::_3,
						 std::placeholders::_4,
						 std::placeholders::_5,
						 std::placeholders::_6,
						 std::placeholders::_7,
						 std::placeholders::_8,
						 std::placeholders::_9,
						 std::placeholders::_10,
						 std::placeholders::_11,
						 std::placeholders::_12,
						 std::placeholders::_13,
						 std::placeholders::_14,
						 std::placeholders::_15,
						 std::placeholders::_16,
						 std::placeholders::_17,
						 std::placeholders::_18,
						 std::placeholders::_19,
						 std::placeholders::_20,
						 std::placeholders::_21,
						 std::placeholders::_22,
						 std::placeholders::_23,
						 std::placeholders::_24,
						 std::placeholders::_25,
						 std::placeholders::_26,
						 std::placeholders::_27,
						 std::placeholders::_28,
						 std::placeholders::_29);
	}

private:
	/**
	 * @brief Deleted constructor to ensure static use of the class.
	 */
	inline BindHelper() noexcept = delete;
};

/**
 * @brief A helper static class used to call bound object member functions by passing arguments in a form of std::tuple.
 * @tparam Args The types of arguments passed when the function is called.
 */
template<typename ...Args>
struct CallHelper {
	/**
	 * @brief Function variation which ensures proper error logging in event a function with more than 29 arguments is attempted to be called.
	 * @param function The function in a std::function wrapper being called.
	 * @param arguments The arguments in a std::tuple being passed as individual arguments to the function.
	 * @return Always false.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept {
		CMF_LOG(CMF, Error, "Unsupported call attempted on a function with more than 31 argument.");
		return false;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 0) {
		if(function == nullptr){
			return false;
		}

		function();

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 1) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 2) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 3) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 4) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 5) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 6) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 7) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 8) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 9) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 10) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 11) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 12) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 13) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 14) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 15) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 16) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 17) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 18) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 19) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 20) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 21) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 22) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 23) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 24) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 25) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments),
				 std::get<24>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 26) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments),
				 std::get<24>(arguments),
				 std::get<25>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 27) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments),
				 std::get<24>(arguments),
				 std::get<25>(arguments),
				 std::get<26>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 28) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments),
				 std::get<24>(arguments),
				 std::get<25>(arguments),
				 std::get<26>(arguments),
				 std::get<27>(arguments));

		return true;
	}

	/**
	 * @brief A helper function used to call a function with arguments contained in a std::tuple.
	 * @param function The function being called in a std::function wrapper.
	 * @param arguments The arguments being passed to the function in a std::tuple.
	 * @return False if function is invalid, true otherwise.
	 */
	inline static constexpr bool call(const std::function<void(Args...)>& function, const std::tuple<Args...>& arguments) noexcept requires (sizeof...(Args) == 29) {
		if(function == nullptr){
			return false;
		}

		function(std::get<0>(arguments),
				 std::get<1>(arguments),
				 std::get<2>(arguments),
				 std::get<3>(arguments),
				 std::get<4>(arguments),
				 std::get<5>(arguments),
				 std::get<6>(arguments),
				 std::get<7>(arguments),
				 std::get<8>(arguments),
				 std::get<9>(arguments),
				 std::get<10>(arguments),
				 std::get<11>(arguments),
				 std::get<12>(arguments),
				 std::get<13>(arguments),
				 std::get<14>(arguments),
				 std::get<15>(arguments),
				 std::get<16>(arguments),
				 std::get<17>(arguments),
				 std::get<18>(arguments),
				 std::get<19>(arguments),
				 std::get<20>(arguments),
				 std::get<21>(arguments),
				 std::get<22>(arguments),
				 std::get<23>(arguments),
				 std::get<24>(arguments),
				 std::get<25>(arguments),
				 std::get<26>(arguments),
				 std::get<27>(arguments),
				 std::get<28>(arguments));

		return true;
	}

private:
	/**
	 * @brief Deleted default constructor ensuring the class can only be used statically.
	 */
	inline CallHelper() noexcept = delete;
};

/**
 * @brief Base event handle interface demanding implementation of probing and scanning functionality from the event handle implementations.
 */
class EventHandleBase {
public:
	/**
	* @brief Default destructor.
	*/
	virtual ~EventHandleBase() = default;

	/**
	 * @brief A function used to check if an event trigger is queued or not.
	 * @param wait The wait time for the check.
	 * @return True if there is a queued event trigger.
	 */
	inline virtual bool probe(TickType_t wait) noexcept = 0;

	/**
	 * @brief A function used to scan and process callbacks of queued event triggers.
	 * @param wait The wait time for the queued event pop.
	 */
	inline virtual void scan(TickType_t wait) noexcept = 0;

	/**
	 * @brief A function used to unblock all threads waiting for this event.
	 */
	inline virtual void unblock() noexcept = 0;

	/**
	 * @return The object owning the handle, responsible for its scanning and lifetime.
	 */
	inline virtual  Object* getOwningObject() const noexcept { return nullptr; }
};

/**
 * @brief Event handle implementation with custom argument types.
 * @tparam Args The types of arguments being broadcast to the callback functions.
 */
template<typename ...Args>
class EventHandle : public EventHandleBase {
public:
	/**
	 * @brief Destructor unregisters this event handle from the owning object, stopping its scanning.
	 */
	inline virtual ~EventHandle() noexcept override {
		if(const Application* app = ApplicationStatics::getApplication()){
			if(EventScanner* scanner = app->getEventScanner()){
				scanner->unregisterHandle(this);
			}
		}

		Object* owningObjectPtr = owningObject.get();
		if(owningObjectPtr == nullptr) {
			return;
		}

		if(Object* owner = owningObjectPtr->getOutermostOwner()) {
			owner->unregisterEventHandle(this);
		}else {
			owningObjectPtr->unregisterEventHandle(this);
		}
	}

	/**
	 * @return The object owning the handle, responsible for its scanning and lifetime.
	 */
	virtual Object* getOwningObject() const noexcept override {
		return *owningObject;
	}

	/**
	 * @brief Bind function binds a member function of an object type instance to the event as a callback.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The instance of object owning the function.
	 * @param function The function reference being bound.
	 * @return Reference to this.
	 */
	template<typename O, typename F>
	inline EventHandle& bind(O* object, F&& function) noexcept {
		if(owningObject.isValid()) {
			Object* owner = owningObject->getOutermostOwner();
			if(owner == nullptr){
				owner = owningObject.get();
			}

			owner->unregisterEventHandle(this);
		}

		owningObject = cast<Object>(object);
		callback = BindHelper<Args...>::template get(object, function);

		Object* owner = owningObject->getOutermostOwner();
		if(owner == nullptr){
			owner = owningObject.get();
		}

		owner->registerEventHandle(this);

		return *this;
	}

	/**
	 * @brief Bind function binds a std::function to the event as a callback paired with the owning object instance.
	 * @tparam O The type of object the function is a member of.
	 * @tparam F The type of function being bound.
	 * @param object The instance of object owning the function.
	 * @param function The function reference being bound.
	 * @return Reference to this.
	 */
	template<typename O, typename F> requires (std::same_as<F, std::function<void(Args...)>> || std::same_as<F, std::function<void(Args...)>&> || std::same_as<F, const std::function<void(Args...)>&>)
	inline EventHandle& bind(O* object, F&& function) noexcept {
		if(owningObject.isValid()) {
			Object* owner = owningObject->getOutermostOwner();
			if(owner == nullptr){
				owner = owningObject.get();
			}

			owner->unregisterEventHandle(this);
		}

		owningObject = object;
		callback = function;

		Object* owner = owningObject->getOutermostOwner();
		if(owner == nullptr){
			owner = owningObject.get();
		}

		owner->registerEventHandle(this);

		return *this;
	}

	/**
	 * @brief Call function queues an argument std::tuple into the call queue for the callback functions to be called with in the next scan call.
	 * @param args The arguments for the next event call.
	 * @return True if successful, false otherwise.
	 */
	inline bool call(const Args&... args) noexcept {
		return callQueue.push(std::tuple<Args...>(args...));
	}

	/**
	 * @brief Function probes for if an event call has been queued and is ready to trigger.
	 * @param wait The maximum wait time for the probing to succeed.
	 * @return True if there is a queued event call, false otherwise.
	 */
	inline virtual bool probe(TickType_t wait) noexcept override {
		std::tuple<Args...> arguments;
		return callQueue.front(arguments, wait);
	}

	/**
	 * @brief Scan function triggers a callback for each triggered event call.
	 * @param wait Maximum wait time for collective calling of all callbacks with all arguments.
	 * When the time is up, callback triggering will abort even if more events remain.
	 */
	inline virtual void scan(TickType_t wait) noexcept override {
		if(!owningObject.isValid()){
			return;
		}

		if(callback == nullptr){
			return;
		}

		while(!callQueue.empty()){
			const uint64_t beginTime = millis();

			std::tuple<Args...> arguments;
			if(!callQueue.pop(arguments, wait)){
				break;
			}

			CallHelper<Args...>::call(callback, arguments);

			wait = std::max((uint64_t)0, (uint64_t) (wait - (millis() - beginTime)));
		}
	}

	/**
	* @brief  Unblock the queue multithreaded safety to unblock all threads waiting on it.
	*/
	inline virtual void unblock() noexcept override {
		callQueue.setKillPill(true);
	}

private:
	WeakObjectPtr<Object> owningObject = nullptr; // Object of which the callback is a member, in case this owning object
	std::function<void(Args...)> callback = nullptr;
	Queue<std::tuple<Args...>> callQueue;
};

#endif //CMF_EVENTHANDLE_H