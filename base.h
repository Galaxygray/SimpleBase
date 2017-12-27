#include <stdlib.h>
#include <iostream>

class SM_Manager;
#ifndef BASE
#define BASE
enum AttrType {
	INT,
	FLOAT,
	STRING
};


enum CompOp {
	NO_OP,                                      // no comparison
	EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
};

//
// Pin Strategy Hint
//
enum ClientHint {
	NO_HINT                                     // default value
};

struct AttrInfo {
	char     *attrName;   /* attribute name       */
	AttrType attrType;    /* type of attribute    */
	int      attrLength;  /* length of attribute  */
};

struct RelAttr {
	char     *relName;    // Relation name (may be NULL)
	char     *attrName;   // Attribute name

						  // Print function
	friend std::ostream &operator<<(std::ostream &s, const RelAttr &ra);
};

struct Value {
	AttrType type;         /* type of value               */
	void     *data;        /* value                       */
						   /* print function              */
	friend std::ostream &operator<<(std::ostream &s, const Value &v);
};

struct Condition {
	RelAttr  lhsAttr;    /* left-hand side attribute            */
	CompOp   op;         /* comparison operator                 */
	int      bRhsIsAttr; /* 1 if the rhs is an attribute,    */
						 /* in which case rhsAttr below is valid;*/
						 /* otherwise, rhsValue below is valid.  */
	RelAttr  rhsAttr;    /* right-hand side attribute            */
	Value    rhsValue;   /* right-hand side value                */
						 /* print function                               */
	friend std::ostream &operator<<(std::ostream &s, const Condition &c);

};

std::ostream &operator<<(std::ostream &s, const CompOp &op);
std::ostream &operator<<(std::ostream &s, const AttrType &at);

//
// Parse function
//


void RBparse(SM_Manager &sm);
#endif // !BASE


