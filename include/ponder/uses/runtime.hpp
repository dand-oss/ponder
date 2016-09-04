/****************************************************************************
**
** This file is part of the Ponder library.
**
** The MIT License (MIT)
**
** Copyright (C) 2009-2014 TEGESO/TEGESOFT and/or its subsidiary(-ies) and mother company.
** Copyright (C) 2016 Bill Quith.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

/*
 *  Runtime uses for Ponder registered data.
 */

#ifndef PONDER_USES_RUNTIME_HPP
#define PONDER_USES_RUNTIME_HPP

//#include <ponder/function.hpp>
#include <ponder/class.hpp>
#include <ponder/constructor.hpp>

namespace ponder {
namespace runtime {
    
namespace impl {
    
//template <typename F>
//static inline const Function* functionCallerFactory(IdRef name, F function)
//{
//    using Traits = detail::FunctionTraits<F>;
//    
//    return new runtime::impl::FunctionCallerImpl<implType, typename Traits::type>(name, function));
//}
    
} // namespace detail
    
    
class ObjectFactory
{
public:
    
    ObjectFactory(const Class &cls) : m_class(cls) {}
    
    const Class& getClass() const { return m_class; }
    
    /**
     * \brief Construct a new instance of the C++ class bound to the metaclass
     *
     * If no constructor can match the provided arguments, UserObject::nothing
     * is returned. If a pointer is provided then placement new is used instead of
     * the new instance being dynamically allocated using new.
     * The new instance is wrapped into a UserObject.
     *
     * \note It must be destroyed with the appropriate destruction function: 
     * Class::destroy for new and Class::destruct for placement new.
     *
     * \param args Arguments to pass to the constructor (empty by default)
     * \param ptr Optional pointer to the location to construct the object (placement new)
     *
     * \return New instance wrapped into a UserObject, or UserObject::nothing if it failed
     * \sa create()
     */
    UserObject construct(const Args& args = Args::empty, void* ptr = nullptr) const; // XXXX rename

    /**
     * \brief Create a new instance of the class bound to the metaclass
     *
     * Create an object without having to create an Args list. See notes for Class::construct().
     * If you need to create an argument list at runtime and use it to create an object then
     * see Class::construct().
     *
     * \param args An argument list.
     *
     * \return New instance wrapped into a UserObject, or UserObject::nothing if it failed
     * \sa construct()
     */
    template <typename ...A>
    UserObject create(A... args) const;    

    /**
     * \brief Destroy an instance of the C++ class bound to the metaclass
     *
     * This function must be called to destroy every instance created with
     * Class::construct.
     *
     * \param object Object to be destroyed
     *
     * \see construct
     */
    void destroy(const UserObject& object) const;
      
    /**
     * \brief Destruct an object created using placement new
     *
     * This function must be called to destroy every instance created with
     * Class::construct.
     *
     * \param object Object to be destroyed
     *
     * \see construct
     */
    void destruct(const UserObject& object) const;
    
private:
    
    const Class &m_class;    
};

        
class FunctionCaller // XXXX docs
{
public:
    
    FunctionCaller(const Function &f);
    
    virtual ~FunctionCaller() {}
    
    const Function& function() const { return m_func; }
    
    /**
     * \brief Call the function
     *
     * \param object Object
     * \param args Arguments to pass to the function, for example "ponder::Args::empty"
     *
     * \return Value returned by the function call
     *
     * \throw ForbiddenCall the function is not callable
     * \throw NullObject object is invalid
     * \throw NotEnoughArguments too few arguments are provided
     * \throw BadArgument one of the arguments can't be converted to the requested type
     */
    Value call(const UserObject& object, const Args& args = Args::empty) const;
    
    /**
     * \brief Call the static function
     *
     * \param args Arguments to pass to the function, for example "ponder::Args::empty"
     *
     * \return Value returned by the function call
     *
     * \throw NotEnoughArguments too few arguments are provided
     * \throw BadArgument one of the arguments can't be converted to the requested type
     */
    Value callStatic(const Args& args = Args::empty) const;
    
protected:
    
    /**
     * \brief Do the actual call
     *
     * This function is a pure virtual which has to be implemented in derived classes.
     *
     * \param object Object
     * \param args Arguments to pass to the function
     *
     * \return Value returned by the function call
     *
     * \throw NullObject object is invalid
     * \throw BadArgument one of the arguments can't be converted to the requested type
     */
    virtual Value execute(const UserObject& object, const Args& args) const;
    
private:
    
    const Function &m_func;
    runtime::impl::FunctionCaller *m_caller;
};


} // namespace runtime
} // namespace ponder

//--------------------------------------------------------------------------------------
// .inl

namespace ponder {
namespace runtime {

template <typename ...A>
inline UserObject ObjectFactory::create(A... args) const
{
    Args a(args...);
    return construct(a);
}
    
} // namespace runtime
} // namespace ponder

//--------------------------------------------------------------------------------------

// define once in client program to instance this
#ifdef PONDER_USES_RUNTIME_IMPL

namespace ponder {
namespace runtime {

    
UserObject ObjectFactory::construct(const Args& args, void* ptr) const
{
    // Search an arguments match among the list of available constructors
    for (std::size_t nb = m_class.constructorCount(), i = 0; i < nb; ++i)
    {
        const Constructor& constructor = *m_class.constructor(i);
        if (constructor.matches(args))
        {
            // Match found: use the constructor to create the new instance
            return constructor.create(ptr, args);
        }
    }
    
    return UserObject::nothing;  // no match found
}
    
void ObjectFactory::destroy(const UserObject& object) const
{
    m_class.destruct(object, false);
    
    const_cast<UserObject&>(object) = UserObject::nothing;
}

void ObjectFactory::destruct(const UserObject& object) const
{
    m_class.destruct(object, true);
    
    const_cast<UserObject&>(object) = UserObject::nothing;
}


FunctionCaller::FunctionCaller(const Function &f)
    :   m_func(f)
    ,   m_caller(std::get<uses::Users::eRuntimeModule>(
                 *reinterpret_cast<const uses::Users::PerFunctionUserData*>(m_func.getUserData())))
{
}

    
Value FunctionCaller::call(const UserObject& object, const Args& args) const
{
    // Check the number of arguments
    if (args.count() < m_func.paramCount())
        PONDER_ERROR(NotEnoughArguments(m_func.name(), args.count(), m_func.paramCount()));

    // Execute the function
    return execute(object, args);
}

Value FunctionCaller::callStatic(const Args& args) const
{
    // Check the number of arguments
    if (args.count() < m_func.paramCount())
        PONDER_ERROR(NotEnoughArguments(m_func.name(), args.count(), m_func.paramCount()));

    // Execute the function
    return execute(UserObject::nothing, args);
}   

Value FunctionCaller::execute(const UserObject& object, const Args& args) const
{
    return m_caller->execute(object, args);
}

    
} // runtime
} // ponder

#endif // PONDER_USES_RUNTIME_IMPL

#endif // PONDER_USES_RUNTIME_HPP