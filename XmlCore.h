#ifndef XMLCORE_H
#define XMLCORE_H

/*
 * grammar:
 doc
    null
    root
    dec root
root
    null
    tag

tag  {tag_begin.id = tag_end.id}
    tag_begin null tagend
    tag_begin tag tagend
    tag_begin cdata tag_end  
    tag_begin val tagend
tag_begin
tag_end 
 */

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <assert.h>
#include "XmlScanner.h"

using std::string;
using std::list;
using std::vector;


struct node 
{
	node (){ parent = children = sibling = NULL;  }
	~node()
	{
		property * cur = properties;
		while( NULL != cur){ properties = cur->next; delete cur; cur = properties;  }
		
		if(children != NULL ) delete children;
		if(sibling != NULL)  delete sibling;
	}
	string tag;
	string value;
	property * properties;
	bool is_end;

	node * parent;
	node * children;
	node * sibling;
};

class XmlCore{
	public:
		XmlCore(){  }
		~XmlCore()
		{ 
			delete m_root;m_root = NULL;
			delete m_dec; m_dec = NULL;
		}
	
		int init(string fname)
		{
			m_cur_node = NULL;
			m_parent_node = NULL;
			m_root = NULL;
			m_dec = NULL;
            
            if(m_scanner.init(fname))
                return -1;

			return 0;
		}

		int parse()
		{  
           return doc();
		}
        
        int cleanup()
        {
            return m_scanner.cleanup();
        }

		void print_tree( node * root,int space )
		{
			//deep first then wide
			if( !root) return ;
			string spaces(space,' ' );
			printf("%stag:[%s] ; is_end:[%d];value[%s];\n ", spaces.c_str(),root->tag.c_str(), root->is_end,root->value.c_str());
			
			print_tree( root->children, space +4);
			print_tree( root->sibling, space);
			return;
		}
		
		node * getRoot(){ return m_root; };
		token * getDec(){ return m_dec; };
		
	private:		
		token * m_dec;
		node * m_root;
		node * m_cur_node;
		node * m_parent_node;
		XmlScanner m_scanner;

        int doc()
		{
			//skip while char
			token * cur = m_scanner.scan();
			//while(cur){ptype(); printf("type:%d morpheme[%s]\n" ,cur->type,cur->get_morpheme()); cur = m_scanner.scan();}
			if(cur == NULL) { printf("empty file\n"); return 0;}
			if(cur->type ==  DECLARE){
//				printf("type[declare]; morpheme [%s]\n",cur->get_property(MORPHEME ) );
				declare( cur );
				//delete cur; note copy to m_dec
				cur = NULL; cur = m_scanner.scan();
			}	
            
            if(cur == NULL)
                return 0;            

			if( cur->type == ROOT_BEGIN){
//				printf("type[ root ]; morpheme [%s]\n",cur->get_property(MORPHEME ) );
				return tag(cur,true);  	
			}
			
			printf("unknown begin tag in the xml begin, error in line %d\n",m_scanner.getcurline());
			return -22;
		}
		
		int declare( token * tok){
            m_dec = tok;return 0;  
        }

		int tag( token *tok, bool is_root = false)
		{
			//match begin ;match value,cdata,tag; match end
			if( NULL == tok) { printf("token is null\n"); return -1; };
			
            if(tag_begin(tok ,is_root) ) { 
				delete tok; tok = NULL; printf("parse tag begin failed\n");return -2 ;
			} else {delete tok; tok = NULL;};

			//match value or cdata
			tok = m_scanner.scan();
			if( NULL== tok) {printf(" get token after tag_begin faild, tag:%s ,line:%d \n", m_cur_node->tag.c_str(), m_scanner.getcurline());  return -3;}; 
			
            if( TAG_VALUE == tok->type || CDATA ==  tok->type){ 
				tag_value_cdata(tok ); delete tok; tok = NULL;
			}
			
            //match parallel tag
			node * pnode = m_parent_node;
			node * cnode = m_cur_node;
			while( tok != NULL && TAG_BEGIN == tok->type){
				m_parent_node = m_cur_node;
				m_cur_node = NULL;
				string ntag = tok->get_morpheme();

				if( tag(tok)){ printf("parse tag failed,tag:%s,parent tag %s,line %d \n",ntag.c_str(),m_parent_node->tag.c_str(),m_scanner.getcurline() );	return -7;}
				else {  tok = NULL; tok = m_scanner.scan();}
				
                m_parent_node = pnode;
				m_cur_node = cnode;
			}	
            
			if( !tok ) tok = m_scanner.scan();
			if( NULL == tok) { printf("tag end is null , tag_begin %s ,line:%d\n", m_cur_node ->tag.c_str(), m_scanner.getcurline()); return -4;}
			if (TAG_END != tok->type ) {  printf("expect end tag[%s  ],near line :%d \n", tok->get_morpheme(),m_scanner.getcurline()); delete tok; return -6; }
		
            if( tag_end(tok) ){ delete tok; return -5;} else delete tok;
			
			return 0;
		} 

        int tag_begin(token * tok ,bool is_root )
		{
            if(SXML_DEBUG) printf("type[tag_begin ]; morpheme [%s] ,line %d\n",tok->get_property(MORPHEME ), m_scanner.getcurline());
	
            if(TAG_BEGIN != tok -> type && ROOT_BEGIN  != tok->type)
			{
				printf("error type %d,should be TAG_BEGIN %d, near line :%d\n", tok->type, TAG_BEGIN, m_scanner.getcurline());
				return -1;
			} 		
			if(is_root)
				if(NULL != m_root) { printf("root duplicate,line %d \n ",m_scanner.getcurline());  return -2;}
			
			node * n = new node;
		    n->tag = tok->get_morpheme();	
			n->is_end = false;	
			n->value = "";
			n->properties = NULL;

			n->parent = m_parent_node;
			n->children = NULL;
			n->sibling = NULL;
			m_cur_node = n;

			if(is_root){ m_root = n;m_parent_node = n;}
		
			//append to dom tree
			if( !is_root ){  
				assert( m_parent_node != NULL); 
				if(NULL != m_parent_node ->children) add_sibling(m_parent_node ->children,n); else m_parent_node ->children = n;
			} 
			
			//printf(" the tree is :\n");
			//print_tree( m_root,0);
			return 0;
		}
	
		int tag_end( token * tok){
			
			if(SXML_DEBUG) printf("type[tag_end]; morpheme [%s], line %d\n",tok->get_property(MORPHEME ),m_scanner.getcurline() );
			
			if( strcmp( (char *)tok->get_morpheme(), (char *) m_cur_node->tag.c_str()))
			{ printf("the start and end of tag not match,end tag:%s,start tag:%s  near line %d\n "
					, tok->get_morpheme(), m_cur_node->tag.c_str(),m_scanner.getcurline() );return -1;  }

			m_cur_node ->is_end = true;
			return 0;
		}

		int tag_value_cdata(token * tok)
		{
            if(SXML_DEBUG) printf("type[tag_value or cdata]; morpheme [%s] ,line %d\n",tok->get_property(MORPHEME ),m_scanner.getcurline() );
			m_cur_node->value = tok->get_morpheme();
			return 0;
		}

	
		void add_sibling(node * header, node * n)
		{
			assert(header && n);
			node * pre = header;
			while(pre ->sibling) pre = pre ->sibling;
			pre->sibling = n;
			return;
		} 
};

#endif
