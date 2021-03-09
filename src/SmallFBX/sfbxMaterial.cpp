#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxMaterial.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Video::getClass() const { return ObjectClass::Video; }
ObjectSubClass Video::getSubClass() const { return ObjectSubClass::Clip; }

void Video::constructObject()
{
    super::constructObject();
    // todo
}

void Video::constructNodes()
{
    super::constructNodes();
    // todo
}



ObjectClass Material::getClass() const { return ObjectClass::Material; }

void Material::constructObject()
{
    super::constructObject();
    // todo
}

void Material::constructNodes()
{
    super::constructNodes();
    // todo
}
} // namespace sfbx
