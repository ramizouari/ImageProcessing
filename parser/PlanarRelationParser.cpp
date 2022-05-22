//
// Created by ramizouari on 15/05/22.
//

#include "PlanarRelationParser.h"

namespace parser {
    PlanarRelationParser::PlanarRelationParser(const std::string &expression, Real width, Real height): ParametricParser(expression, "x", "y"), width(width), height(height)
    {
        parser.DefineVar("i", &indexesReal[0]);
        parser.DefineVar("j", &indexesReal[1]);
        parser.DefineVar("u", &normalizedVariables[0]);
        parser.DefineVar("v", &normalizedVariables[1]);
        parser.DefineVar("r", &polarVariables[0]);
        parser.DefineVar("t", &polarVariables[1]);
        addConstant("n",width);
        addConstant("m",height);
        addConstant("size",width*height);
        addConstant("N",width);
        addConstant("M",height);
        addConstant("S",width*height);
    }

    void PlanarRelationParser::setParameter(int index, Real val, int pos,bool updateDependetVariables) {
        if (index == 0) {
            normalizedVariables[0] = 2*val/width-1;
            variables[0] = val-width/2;
        } else {
            normalizedVariables[1] = 2*val/height-1;
            variables[1] = val-height/2;
        }
        indexes[index]=pos;
        indexesReal[index] = pos;
        std::complex<Real> z(variables[0], variables[1]);
        polarVariables[0] = std::abs(z);
        polarVariables[1] = std::arg(z);
        if(updateDependetVariables)
            this->updateDependentVariables();
    }
} // parser