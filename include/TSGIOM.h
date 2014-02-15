#ifndef TSGIOM_H
#define TSGIOM_H
#include "GIOM.h"

/**
 * Time-Sequential Generalized Input Output Model
 */
class TSGIOM: public GIOM
{
    public:
        TSGIOM();
        virtual ~TSGIOM();
        virtual void Update();    // reimplement Update, add time sequence
    protected:
        virtual OSpace Restrict(Input, OSpace &);    // reimplement Restrict
        /* bring in the time sequence feature */
        Input pre_in; /**< previous input value */
        Output pre_out; /**< previous output value */
    private:
};

#endif // TSGIOM_H
