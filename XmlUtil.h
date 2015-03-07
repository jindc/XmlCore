#ifndef XML_UTIL_H
#define XML_UTIL_H

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <assert.h>
#include "XmlCore.h"

using std::string;
using std::list;
using std::vector;

struct trans_node
{
	vector<string> args;
	void * rp;
};

class XmlUtil{
	public:
		XmlUtil(){}
		~XmlUtil(){}
	    void testXmlCore( string fname)
		{
            XmlCore core;
			int ret ;

            ret = core.init(fname);
			assert(!ret);

			ret = core.parse();
			assert(!ret);
		
			node * root = core.getRoot();
			printf(" the tree is :\n");
			core.print_tree( root ,0);

			//token * dec = getDec();
            core.cleanup();
		}	

		void traverse( node * root, int (* op)(node * n,void * arg  ),void * arg  )
		{
			//deep first then wide
			if( !root) return ;
			if( op(root, arg)) return;

			traverse( root->children, op, arg  );
			traverse( root->sibling, op ,arg );
			return;
		}
		
		static int get_by_tag(node * n ,void* arg)
		{
			trans_node * tn = ( trans_node * )arg;
			vector<node * > * r = ( vector <node *> *)tn->rp;
			if( (tn->args).size() !=1) return -1 ;
			if( !strcmp( n->tag.c_str(), (tn->args[0]).c_str()))
			{
				r->push_back(n);
			}
			return 0;
		} 
		
		vector<node *> getNodesByTag(node * root,string tag)
		{
			vector <node *> values;
			trans_node tn;
			tn.args.push_back(tag);
			tn.rp = (void *)&values;
			traverse(root,get_by_tag,(void *) &tn);
			return values;
		} 
};
#endif
