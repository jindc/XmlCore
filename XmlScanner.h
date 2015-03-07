#ifndef XML_SCANNER_H
#define XML_SCANNER_H

/*
 * lexical
dec 
    <?xml dec_props  ?>
dec_props
    null
    dec_prop dec_props
dec_prop
    id="chars"
comnent
    <!-- chars -->|<!-- comment -->
tag_begin
    <id tag_prop>
tag_value
    chars
cdata
    <![CDATA[ chars ]]> 
tag_end
    </id>
id
    chars
tag_prop
    chars
 * */

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <assert.h>

using std::string;
using std::list;
using std::vector;

#define MORPHEME "morpheme"
#define S_CDATA_BEGIN "<![CDATA["
#define S_CDATA_END "]]>"
#define SXML_DEBUG 0

enum  token_type{DECLARE,ROOT_BEGIN,TAG_BEGIN, TAG_END,TAG_VALUE,	CDATA};

struct property
{
	property * next;
	string name;
	string value;
};

struct token
{
	token(){ properties = NULL; }
	~token()
	{
		property * cur = properties;
		while( NULL != cur){ properties = cur->next; delete cur; cur = properties;  }
	}
	token_type type;
	property * properties;

	const char * get_property(char * name)
	{
		property * temp = properties;
		while( temp != NULL)
		{
			if(! strcmp( (char *)temp->name.c_str(),name))
				return temp->value.c_str();
		}
		return NULL;
	}
	
	void set_property(char *name, char * value)
	{
		property* p = new property;
		p->name = string(name);
		p->value = string(value);
		p->next = properties ;
		properties = p;
	}
	const char * get_morpheme(){ return get_property(MORPHEME ); }
	void set_morpheme(char * value){ set_property(MORPHEME,value);};

};

struct char_cache
{
	char_cache(){ len = 0; start = 0; max_len = 64;};
	int len;
	int start;
	int max_len;
	char buffer[64];
	
	void put_char( char c) {buffer[ (start + len++)%max_len] = c;  }
	void put_charp(char * s)
	{
		assert( (int)strlen(s) <= max_len -len);
		char * t = s;
		while(  *t )
		{ put_char(*t);t++; }
	}

	int get_char(){ if(len<= 0) return 257; else { char c = buffer[start++];start %=max_len;len--; return c; }} ;
};

class XmlScanner{
	public:
		XmlScanner(){}
		~XmlScanner(){}
		
		int init(string fname)
		{
			m_line = 1;
			m_nextline = 1;
			m_root_trigger = false;
			m_fname = fname;
			
            m_file  = fopen(fname.c_str(),"rb");
			if( NULL == m_file )
                return -1;
			
			return 0;
		}
        
        int cleanup()
        {
            if(m_file != NULL)
                fclose(m_file);
            return 0;
        }

        int getcurline()
        {
            return m_line;
        }
		token * scan()
		{
			m_line = m_nextline;
			while(1)
			{
                int ret = readchar();
                if(ret == 0){
                    return NULL;
                }
				if('\t' ==  m_peek || ' '== m_peek || '\r'== m_peek)
					continue;
				if('\n' == m_peek){ m_nextline++; continue;}
				break;
			}
			
			string cache;
			switch( m_peek){
				case '<':
                    do{ 
                        if(1 != readchar()) 
                            return NULL;
                    } while(0);

					cache.append(1,m_peek);
                    if('?' == m_peek)	{
						token *t = new token;
						t->type = DECLARE;
						string morpheme("<?");
						if(read_until_s("?>",morpheme))
						{
							printf("read xml declare error:can not find declare end:line %d\n",m_line);
							return NULL;
						}
						t->set_morpheme((char * )morpheme.c_str() );
						return t;
					}else if( '/' == m_peek){
						string morpheme;
						if( read_until_s(">",morpheme))
						{
							printf("read xml end char [>] failed,line:%d\n",m_line);
							return NULL;
						}
						token *t = new token;
						t->type = TAG_END;
						string clear_morpheme(morpheme.c_str(),morpheme.size() - 1); 
						t->set_morpheme( (char *) clear_morpheme.c_str());
						return t;

					}else if('!' == m_peek)
					{//cdata
						string buffer;
						//comment<!-- -->
						if( readnch(strlen("<!--") -2,buffer) || strcmp("--", buffer.c_str()))
						{
							m_char_cache.put_charp((char *) buffer.c_str());
						}else{
							string morpheme;
							if( read_until_s( "-->",morpheme)){
								printf("read xml comment end flag error:can not find:line %d \n",m_line);
								return NULL;
							}
							return scan();
						}
				
						//cdata
						buffer.clear();	
						if(readnch(strlen(S_CDATA_BEGIN) -2,buffer) ||
								strcmp(S_CDATA_BEGIN+2,(char *) buffer.c_str())
							)
						{
							m_char_cache.put_charp((char *) buffer.c_str());
						}else{
							token * t = new token;
							t->type = CDATA;
							string morpheme;
							if( read_until_match(S_CDATA_BEGIN,S_CDATA_END,morpheme)){
								printf("read xml cdata end flag error:can not find:line %d \n",m_line);
								return NULL;
							};
							string clear_morpheme(morpheme.c_str(),morpheme.size() - strlen(S_CDATA_END)); 
							t->set_morpheme( (char *) clear_morpheme.c_str());
							increase_line( clear_morpheme.c_str());
							return t;
						}
					}//end cdata
					
					//tag
					{
						string morpheme(1,cache.c_str()[0]);
						if( read_until_s(">",morpheme))
						{
							printf("read xml end char [>] failed,line:%d\n",m_line);
							return NULL;
						}
						token *t = new token;
						if( m_root_trigger)t->type = TAG_BEGIN; else t->type = ROOT_BEGIN; 
						m_root_trigger = true;

						string clear_morpheme(morpheme.c_str(),morpheme.size() - 1); 
						t->set_morpheme( (char *) clear_morpheme.c_str());
						return t;
					}		
					assert(0);
				default:
					//TAG_VALUE
					token *t = new token;
					t->type = TAG_VALUE;
					string morpheme;
					morpheme.append(1,m_peek);
					if(read_until_s("<",morpheme))
					{
						printf("read xml value error:can not find start tag:line %d\n",m_line);
						return NULL;
					}
					m_char_cache.put_char('<');
					t->set_morpheme((char *) morpheme.erase( morpheme.size()-1,strlen("<") ).c_str());
					increase_line( morpheme.c_str());
					return t;	
			}
			return NULL;
		}
	

	private:		
		string m_fname;
		FILE * m_file;
		char m_peek;
		int m_line;
		int m_nextline;
		char_cache m_char_cache;
		bool m_root_trigger;

		//from next start
		int read_until_s(string chars,string &s)
		{
			while( chars.size() > s.size() 
					|| strcmp(chars.c_str(),s.c_str() +( s.size()- chars.size() ))
					){
				if( 1 != readchar()) return -1;
				s.append(1,m_peek);
			}
			return 0;
		}
		
		//begin not equal end
		int read_until_match(string begin, string end,string &have_read)
		{
			int deep = 0;
			int begin_len = begin.size(),end_len = end.size();
			while(1) 
			{
				if( 0 == strcmp(end.c_str(),have_read.c_str() + (have_read.size() - end_len ))){
					if(deep){ --deep; } else return 0;
				}

				if( 0 ==  strcmp( begin.c_str(), have_read.c_str() + (have_read.size() - begin_len)))
						++deep;
				if( 1 != readchar()) return -1;
				have_read.append(1,m_peek);
			}
			return 0;
		}
		
		int readnch(int n, string &s)
		{
			while(n--)
			{
				if( 1 != readchar())
					return -1;
				s.append(1,m_peek);
			}	
			return 0;
		}

		void increase_line( const char * s)
		{
			while(*s){ if ( '\n' == *s) m_nextline++; s++;};
		}

		int readchar()
		{ 
			int c = 257;
			if( (c= m_char_cache.get_char()) < 257)	
			{ 
				m_peek = (char )c; return 1;
			}
			else
				return fread( (void *)(&m_peek), 1,1,m_file);
			return 0;
		}	
};

#endif
