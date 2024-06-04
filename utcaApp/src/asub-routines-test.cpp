#include <epicsUnitTest.h>
#include <testMain.h>

#include "asub-routines.cpp"

class PosCalcSelfTest {
    aSubRecord prec;

    /* inputs */
    epicsInt32 a, b, c, d;
    epicsFloat64 x_off, x_gain, y_off, y_gain, s_gain, q_off, q_gain;

    /* outputs */
    epicsFloat64 x, y, s, q;

    void initialize_record()
    {
        auto set_field = [](auto &value, auto type, auto &value_field, auto &size_field, auto &type_field) {
            value_field = &value;
            size_field = 1;
            type_field = type;
        };

        set_field(a, menuFtypeLONG, prec.a, prec.nea, prec.fta);
        set_field(b, menuFtypeLONG, prec.b, prec.neb, prec.ftb);
        set_field(c, menuFtypeLONG, prec.c, prec.nec, prec.ftc);
        set_field(d, menuFtypeLONG, prec.d, prec.ned, prec.ftd);

        set_field(x_off, menuFtypeDOUBLE, prec.e, prec.nee, prec.fte);
        set_field(x_gain, menuFtypeDOUBLE, prec.f, prec.nef, prec.ftf);
        set_field(y_off, menuFtypeDOUBLE, prec.g, prec.neg, prec.ftg);
        set_field(y_gain, menuFtypeDOUBLE, prec.h, prec.neh, prec.fth);
        set_field(s_gain, menuFtypeDOUBLE, prec.i, prec.nei, prec.fti);
        set_field(q_off, menuFtypeDOUBLE, prec.j, prec.nej, prec.ftj);
        set_field(q_gain, menuFtypeDOUBLE, prec.k, prec.nek, prec.ftk);

        set_field(x, menuFtypeDOUBLE, prec.vala, prec.nova, prec.ftva);
        set_field(y, menuFtypeDOUBLE, prec.valb, prec.novb, prec.ftvb);
        set_field(s, menuFtypeDOUBLE, prec.valc, prec.novc, prec.ftvc);
        set_field(q, menuFtypeDOUBLE, prec.vald, prec.novd, prec.ftvd);
    }

    void set_values()
    {
        a = 100;
        b = 210;
        c = 350;
        d = 420;

        x_off = 1000;
        x_gain = 190;
        y_off = -1000;
        y_gain = 185;
        s_gain = 0.45;
        q_off = 9000;
        q_gain = 170;
    }

    void check_outputs()
    {
        asub_pos_calc(&prec);
        auto compare_expected_results = [](auto value, auto expected, auto name) {
            testOk(std::abs(value - expected) < .01, "PosCalc verification for %s", name);
        };

        compare_expected_results(x, -1021.1111, "X");
        compare_expected_results(y, 917.7778, "Y");
        compare_expected_results(s, 486, "Sum");
        compare_expected_results(q, -9037.8886, "Q");
    }

  public:
    PosCalcSelfTest()
    {
        initialize_record();
        set_values();
        check_outputs();
    }
};

MAIN(asubRoutinesTest)
{
    testPlan(4);

    PosCalcSelfTest();
}
