#ifndef REDUCE_PROPERTY_H
#define REDUCE_PROPERTY_H

#include <string>
#include <memory>
#include "Property.h"


namespace fig{

/**
 * @brief Reduce the @idx parsed property and reduce it to a new property
 *        that makes sence to check in @module. FIXME explain better. 
 */
std::shared_ptr<Property>
reduceProperty(unsigned int idx, const std::string &module);
    
}


#endif // REDUCE_PROPERTY_H
