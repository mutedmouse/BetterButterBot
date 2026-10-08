#ifndef CMF_CAST_H
#define CMF_CAST_H

class Object;

/**
 * @brief Casts a given object to another type, if they are compatible and safe to cast between them.
 * This is a non-const pointer version of the function.
 * @tparam T1 Type of object being cast to.
 * @tparam T2 Type of object being cast from.
 * @param object Object instance being cast.
 * @return The pointer to the object of new type,
 * or nullptr if no possible to cast between the two types for the given instance of object.
 */
template<typename T1, typename T2, typename = std::enable_if<std::derived_from<T1, Object>, T1>::type, typename = std::enable_if<std::derived_from<T2, Object>, T2>::type>
inline static T1* cast(T2* object) noexcept{
	if(object == nullptr){
		return nullptr;
	}

	if(object->template isA<T1>()){
		return (T1*) object;
	}

	return nullptr;
}

/**
 * @brief Casts a given object to another type, if they are compatible and safe to cast between them.
 * This is a const pointer version of the function.
 * @tparam T1 Type of object being cast to.
 * @tparam T2 Type of object being cast from.
 * @param object Object instance being cast.
 * @return The pointer to the object of new type,
 * or nullptr if no possible to cast between the two types for the given instance of object.
 */
template<typename T1, typename T2, typename = std::enable_if<std::derived_from<T1, Object>, T1>::type, typename = std::enable_if<std::derived_from<T2, Object>, T2>::type>
inline static const T1* cast(const T2* object) noexcept{
	if(object == nullptr){
		return nullptr;
	}

	if(object->template isA<T1>()){
		return (const T1*) object;
	}

	return nullptr;
}

#endif //CMF_CAST_H