#include "yl_mmu_utility_helper.h"

#include <stdio.h>
#include <string.h>

void Dump_Usage_Map( char buf[], FILE *fp )
{
    char c          = '\0';
    int i           = 0;
    int line_width  = 128;

    while ( (c = *buf++) )
    {
        fputc( c, fp );

        if ( ++i == line_width )
        {
            i = 0;
            fputc( '\n', fp );
        }
    }

    fputc( '\n', fp );
}