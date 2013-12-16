#ifndef PYTHONAPI_PYVARIANT_H
#define PYTHONAPI_PYVARIANT_H

#include "pythonapi_object.h"
#include <memory>

//Qt typedefs
typedef unsigned int uint;
typedef qint64 qlonglong;

class QVariant;

namespace pythonapi{

    class PyVariant: public Object{
    public:
        PyVariant();
        PyVariant(PyVariant& pv);
        PyVariant(QVariant* data);//takes ownership of this QVariant instance

        bool isValid();

        //QVartiant constructor wrapper
        PyVariant(qlonglong value);
        PyVariant(double value);
        PyVariant(const char* value);

        ~PyVariant();
        void __del__();
        const char* __str__();
        qlonglong __int__();
        double __float__();
        bool __bool__() const;
        IlwisTypes ilwisType();

        QVariant& data();

        /**
        * \brief toPyVariant provides static_cast for the return Value of the Engine.do() method on Python side
        * \param obj takes the return value of Engine::_do() which is of the general type pythonapi::Object
        * \return castet pointer to the same object as PyVariant*
        */
        static PyVariant* toPyVariant(Object* obj);


    protected:
        std::unique_ptr<QVariant> _data;
    };

}

#endif // PYTHONAPI_PYVARIANT_H
