#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxMaterial.h"
#include "sfbxDocument.h"

namespace sfbx {


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
