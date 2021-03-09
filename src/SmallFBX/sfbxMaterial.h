#pragma once
#include "sfbxObject.h"

namespace sfbx {

// material

class Material : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

protected:
    void constructObject() override;
    void constructNodes() override;
};


} // namespace sfbx
