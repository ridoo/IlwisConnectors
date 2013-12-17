#ifndef PYTHONAPI_ILWISOBJECT_H
#define PYTHONAPI_ILWISOBJECT_H

#include <memory>
#include "pythonapi_object.h"

namespace Ilwis {
    class IlwisObject;

    template<class T> class IlwisData;
    typedef IlwisData<IlwisObject> IIlwisObject;
}

namespace pythonapi {

    class IlwisObject: public Object{
        friend class Engine;
    public:
        //should be the same as enum Ilwis::IlwisObject::ConnectorMode (ilwisobject.h)
        enum ConnectorMode{cmINPUT=1, cmOUTPUT=2, cmEXTENDED=4};
        enum StoreMode{smMETADATA=1, smBINARYDATA=2};

    protected:
        IlwisObject(Ilwis::IIlwisObject* object);
        std::shared_ptr<Ilwis::IIlwisObject> _ilwisObject;
        std::shared_ptr<Ilwis::IIlwisObject> ptr() const;
    public:
        IlwisObject();
        virtual ~IlwisObject();

        bool connectTo(const char* url, const char* format  = "", const char* fnamespace = "", ConnectorMode cmode = cmINPUT);
        bool store(int storeMode = smMETADATA | smBINARYDATA);
        bool __bool__() const;
        const char *__str__();
        const char *__add__(const char* value);
        const char *__radd__(const char* value);
        const char *name();
        void name(const char* name);
        const char *type();
        quint64 ilwisID() const;
        IlwisTypes ilwisType();
    };

}

#endif // PYTHONAPI_ILWISOBJECT_H
