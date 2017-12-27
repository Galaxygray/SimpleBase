#include "parser_internal.h"
#include "SystemManager.h"
#include <string.h>
#include <iostream>
using namespace std;
const int maxnattrs = 100;



ostream &operator<<(ostream &s, const AttrInfo &ai)
{
	return
		s << " attrName=" << ai.attrName
		<< " attrType=" <<
		(ai.attrType == INT ? "INT" :
			ai.attrType == FLOAT ? "FLOAT" : "STRING")
		<< " attrLength=" << ai.attrLength;
}

ostream &operator<<(ostream &s, const RelAttr &qa)
{
	return
		s << (qa.relName ? qa.relName : "NULL")
		<< "." << qa.attrName;
}

ostream &operator<<(ostream &s, const Condition &c)
{
	s << "\n      lhsAttr:" << c.lhsAttr << "\n"
		<< "      op=" << c.op << "\n";
	if (c.bRhsIsAttr)
		s << "      bRhsIsAttr=TRUE \n      rhsAttr:" << c.rhsAttr;
	else
		s << "      bRshIsAttr=FALSE\n      rhsValue:" << c.rhsValue;
	return s;
}

ostream &operator<<(ostream &s, const Value &v)
{
	s << "AttrType: " << v.type;
	switch (v.type) {
	case INT:
		s << " *(int *)data=" << *(int *)v.data;
		break;
	case FLOAT:
		s << " *(float *)data=" << *(float *)v.data;
		break;
	case STRING:
		s << " (char *)data=" << (char *)v.data;
		break;
	}
	return s;
}

ostream &operator<<(ostream &s, const CompOp &op)
{
	switch (op) {
	case EQ_OP:
		s << " =";
		break;
	case NE_OP:
		s << " <>";
		break;
	case LT_OP:
		s << " <";
		break;
	case LE_OP:
		s << " <=";
		break;
	case GT_OP:
		s << " >";
		break;
	case GE_OP:
		s << " >=";
		break;
	case NO_OP:
		s << " NO_OP";
		break;
	}
	return s;
}

ostream &operator<<(ostream &s, const AttrType &at)
{
	switch (at) {
	case INT:
		s << "INT";
		break;
	case FLOAT:
		s << "FLOAT";
		break;
	case STRING:
		s << "STRING";
		break;
	}
	return s;
}


int GetAttrLength(AttrType &attrType, char *attrname) {
	if (strncmp(attrname, "INT", 3) == 0) {
		attrType = INT;
		return 4;
	}
	else if(strncmp(attrname, "FLOAT", 3) == 0){
		attrType = FLOAT;
		return 4;
	}
	else {
		attrType = STRING;
		int len = 4, value = 0;
		while (isdigit(attrname[len])) {
			value = value * 10 + attrname[len] - '0';
			len++;
		}
		return value;
	}
}


void GetAttrList(NODE *List, AttrInfo *attrInfoList, int &nattrs) {
	NODE *p;
	nattrs = 0;

	while (List != NULL) {
		p = List->u.LIST.curr;
		attrInfoList[nattrs].attrName = p->u.ATTRTYPE.attrname;
		attrInfoList[nattrs].attrLength = GetAttrLength(attrInfoList[nattrs].attrType, p->u.ATTRTYPE.type);
		nattrs++;
		
		List = List->u.LIST.next;
	}
}

void GetReltionAttrs(NODE *List, RelAttr *relAttr, int &relattrs) {
	NODE *p;
	relattrs = 0;

	while (List != NULL) {
		p = List->u.LIST.curr;
		relAttr[relattrs].relName = p->u.RELATTR.relname;
		relAttr[relattrs].attrName = p->u.RELATTR.attrname;
		relattrs++;

		List = List->u.LIST.next;
	}
}

void GetReltions(NODE *List, char **reltions, int &nrels) {
	NODE *p;
	nrels = 0;
	
	while (List != NULL) {
		p = List->u.LIST.curr;
		reltions[nrels] = p->u.RELATION.relname;
		nrels++;

		List = List->u.LIST.next;
	}
}

void GetConditions(NODE *List, Condition *conditions, int &nCondtions) {
	NODE *p;
	nCondtions = 0;
	while (List != NULL) {
		p = List->u.LIST.curr;
		conditions[nCondtions].lhsAttr.attrName = p->u.CONDITION.lhsRelattr->u.RELATTR.attrname;
		conditions[nCondtions].lhsAttr.relName = p->u.CONDITION.lhsRelattr->u.RELATTR.relname;
		conditions[nCondtions].op = p->u.CONDITION.op;
		if (p->u.CONDITION.rhsRelattr) {
			conditions[nCondtions].bRhsIsAttr = true;
			conditions[nCondtions].rhsAttr.attrName = p->u.CONDITION.rhsRelattr->u.RELATTR.attrname;
			conditions[nCondtions].rhsAttr.relName = p->u.CONDITION.rhsRelattr->u.RELATTR.relname;
		}
		else {
			conditions[nCondtions].bRhsIsAttr = false;
			conditions[nCondtions].rhsValue.type = p->u.CONDITION.rhsValue->u.VALUE.type;
			switch (conditions[nCondtions].rhsValue.type) {
				case INT :
					conditions[nCondtions].rhsValue.data = &p->u.CONDITION.rhsValue->u.VALUE.ival;
					break;
				case FLOAT :
					conditions[nCondtions].rhsValue.data = &p->u.CONDITION.rhsValue->u.VALUE.rval;
					break;
				case STRING :
					conditions[nCondtions].rhsValue.data = p->u.CONDITION.rhsValue->u.VALUE.sval;
			}
		}
		nCondtions++;

		List = List->u.LIST.next;
	}
}

void GetValues(NODE *List, Value *values, int &nValue) {
	nValue = 0;
	NODE *p;

	while (List != NULL) {
		p = List->u.LIST.curr;
		values[nValue].type = p->u.VALUE.type;
		switch (values[nValue].type) {
		case INT:
			values[nValue].data = &p->u.VALUE.ival;
			break;
		case FLOAT:
			values[nValue].data = &p->u.VALUE.rval;
			break;
		case STRING:
			values[nValue].data = p->u.VALUE.sval;
		}

		nValue++;
		List = List->u.LIST.next;
	}
}


void Analysis(NODE *root, SM_Manager &sm) {
	switch(root->kind) {
		case N_CREATETABLE :
		{
			int nattrs;
			AttrInfo atrrinfos[maxnattrs];
			GetAttrList(root->u.CREATETABLE.attrlist, atrrinfos, nattrs);
			cout << "TableName: " << root->u.CREATETABLE.relname << endl;
			for (int i = 0; i < nattrs; i++) {
				cout << atrrinfos[i] << endl;
			}
			sm.CreateTable(root->u.CREATETABLE.relname, nattrs, atrrinfos);
			break;
		}
		case N_CREATEINDEX :
		{

			break;
		}
		case N_DROPTABLE :
		{
			cout << root->u.DROPTABLE.relname << endl;
			sm.DropTable(root->u.DROPTABLE.relname);
			break;
		}
		case N_QUERY :
		{
			int       nSelAttrs = 0;
			RelAttr  relAttrs[maxnattrs];
			int       nRelations = 0;
			char      *relations[maxnattrs];
			int       nConditions = 0;
			Condition conditions[maxnattrs];

			GetReltionAttrs(root->u.QUERY.relattrlist, relAttrs, nSelAttrs);
			GetReltions(root->u.QUERY.rellist, relations, nRelations);
			GetConditions(root->u.QUERY.conditionlist, conditions, nConditions);

			/*for (int i = 0; i < nSelAttrs; i++)
				cout << relAttrs[i] << endl;
			for (int i = 0; i < nRelations; i++)
				cout << relations[i] << endl;
			for (int i = 0; i < nConditions; i++)
				cout << conditions[i] << endl;*/



			if (sm.Select(nSelAttrs, relAttrs, nRelations, relations[0], nConditions, conditions))
				cout << "Erro!" << endl;
			break;
		}
		case N_INSERT :
		{
			int nValues = 0;
			Value values[maxnattrs];

			GetValues(root->u.INSERT.valuelist, values, nValues);
			/*cout << "TableName: " << root->u.INSERT.relname << endl;
			for (int i = 0; i < nValues; i++) {
				cout << values[i] << endl;
			}*/

			sm.Insert(root->u.INSERT.relname, nValues, values);
			break;
		}
		case N_UPDATE :
		{
			RelAttr relAttr;

			// The RHS can be either a value or an attribute
			Value rhsValue;
			RelAttr rhsRelAttr;
			int bIsValue;

			int nConditions = 0;
			Condition conditions[maxnattrs];

			relAttr.attrName = root->u.UPDATE.relattr->u.RELATTR.attrname;
			relAttr.relName = root->u.UPDATE.relattr->u.RELATTR.relname;
			if (root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.relattr) {
				bIsValue = true;
				rhsRelAttr.relName = root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.relattr->u.RELATTR.relname;
				rhsRelAttr.attrName = root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.relattr->u.RELATTR.attrname;
			}
			else {
				bIsValue = false;
				rhsValue.type = root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.value->u.VALUE.type;
				switch (rhsValue.type) {
				case INT:
					rhsValue.data = &root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.value->u.VALUE.ival;
					break;
				case FLOAT:
					rhsValue.data = &root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.value->u.VALUE.rval;
					break;
				case STRING:
					rhsValue.data = root->u.UPDATE.relorvalue->u.RELATTR_OR_VALUE.value->u.VALUE.sval;
				}
			}
			GetConditions(root->u.UPDATE.conditionlist, conditions, nConditions);

			/*cout << relAttr << endl;
			cout << rhsValue << endl;
			for (int i = 0; i < nConditions; i++) {
				cout << conditions[i] << endl;
			}*/
			sm.Update(root->u.UPDATE.relname, nConditions, conditions, relAttr, rhsValue);
			break;
		}
		case N_DELETE :
		{
			int nConditions = 0;
			Condition conditions[maxnattrs];

			GetConditions(root->u.DELETE.conditionlist, conditions, nConditions);

			/*cout << "TableName: " << root->u.DELETE.relname << endl;
			for (int i = 0; i < nConditions; i++)
				cout << conditions[i] << endl;*/

			sm.Delete(root->u.DELETE.relname, nConditions, conditions);
			break;
		}
	}
}