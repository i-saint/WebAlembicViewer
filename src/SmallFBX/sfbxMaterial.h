#pragma once
#include "sfbxObject.h"

namespace sfbx {

// texture & material

// Video represents texture data
class Video : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    ObjectSubClass getSubClass() const override;

protected:
    void constructObject() override;
    void constructNodes() override;
};

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
