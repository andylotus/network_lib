#ifndef LOTUS_BASE_COPYABLE_H
#define LOTUS_BASE_COPYABLE_H

namespace lotus
{

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
};

};

#endif  // LOTUS_BASE_COPYABLE_H
