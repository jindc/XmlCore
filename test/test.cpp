#include "XmlUtil.h"

int main(int argc, char * argv[]  )
{
    if(argc != 2){
        printf("usage:xmltest <xmlfile>\n");
        return -1;
    }
	XmlUtil util;
	util.testXmlCore(argv[1]);
}

