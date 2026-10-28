#include "KeywordProcessor.h"

#include "PageProcessor.h"
#include <iostream>


int main()
{
    KeyWordProcessor words;
    words.process("../corpus/CN", "../corpus/EN");
    
    PageProcessor pages;
    pages.process("../corpus/webpages");          // RSS 在这个目录
    
    std::cout << "dict and pages done" << std::endl;
    return 0;
}