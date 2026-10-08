#ifndef CMF_SINGLETON_H
#define CMF_SINGLETON_H

#include "Object/Object.h"
#include "Object/Class.h"

/**
 * @brief Empty singleton class used to ensure all singletons will be objects.
 * Singletons are stored in the Application instance and exactly one of each type can exist at all times within the application.
 */
class Singleton : public Object {
    // TODO add a constructor that will check if the singleton type already exists in the application, and will log error and abort if it exists.
	GENERATED_BODY(Singleton, Object, void)
};

#endif //CMF_SINGLETON_H