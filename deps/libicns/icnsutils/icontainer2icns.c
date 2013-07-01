/*
File:       icontainer2icns.cpp
Copyright (C) 2005 Thomas Lübking <baghira-style@gmx.net>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
Boston, MA 02110-1301, USA.
*/
#include <stdio.h>
#include <string.h>

// --> find icns data[i]
// --> data[i-9] - data[i-1] = "[######]" (id)
// --> data[i-58] - data[i-58+x] = "name"

static inline unsigned short clamp(short s) {
   if (s < 0) return 118 + s;
   if (s > 117) return s - 118;
   return s;
}

static inline int isChar(char c) {
   if (c > 'z') return 0;
   if (c < 'a') return (c == '_');
   return -1;
}

int main(int argc, char **argv)
{
   printf("icontainer2icns, (C) 2005/2007 by Thomas Lübking\n\n");
   if (argc < 2) {
      printf("\nusage: icontainer2icns foo.icontainer\n\n");
      return -1;
   }
   else if (argc > 2) {
      printf("\nusage: icontainer2icns foo.icontainer\nif your icontainer file contains spaces etc.,\nuse a system valid form (i.e. use \"my foo.icontainer\" or my\\ foo.icontainer)\n\n");
      return -1;
   }
   char buffer[118];
   FILE *icontainer = NULL;
   FILE *icns = NULL;
   short i = 0;
   
   //int iconCounter = 0;
   int c;
   char name[256];

   if( (icontainer = fopen(argv[1], "r")) == NULL ) {
      printf("error while opening file %s\n",argv[1]);
      return -1;
   }
   
   rewind(icontainer);
   
   while ((c = getc(icontainer)) != EOF) {
      if (c != 'i') {
         buffer[i] = (char)(c & 0xff);
         i = clamp(++i);
         if (icns)
            fputc(buffer[i], icns);
      }
      else { // wow, lets test if this starts a new icns!
         if ((c = getc(icontainer)) == 'c') {
         if ((c = getc(icontainer)) == 'n') {
         if ((c = getc(icontainer)) == 's') { // yupp!
            if (icns) fclose(icns); // buffer only contains new icns info
            // generate the name
            // i - 7 .. i - 2 is the icontainer id
            // could be 56-4, so no memcpy please!
//             printf("%s\n",buffer);
            short j;
            short k = 0;
            if (buffer[clamp(i-1)] == ']') {
               j = clamp(i-7);
               for (k = 0; k < 6; j = clamp(++j)) {
                  name[k++] = buffer[j];
               }
               name[k++] = '-';
            }
            // now the name
            j = clamp(i+1);
            while (!isChar(buffer[j]) && j != i) j = clamp(++j);
            while (isChar(buffer[j]) && j != i) {
               name[k++] = buffer[j]; j = clamp(++j);
            }
            name[k++] = '.'; name[k++] = 'i'; name[k++] = 'c';
            name[k++] = 'n'; name[k++] = 's'; name[k] = '\0';
            
//             printf("%s\n",name);
                  
            if( (icns = fopen(name, "w")) == NULL ) {
               printf("error while opening icns file %s\n", name);
               return -1;
            }
            
            rewind(icns);
            // write header
            fputc('i', icns); fputc('c', icns);
            fputc('n', icns); fputc('s', icns);
            // init buffer
            for (i = 0; i < 117; ++i)
               buffer[i] = (char)(getc(icontainer) & 0xff);
         }
         else { // reset
            ungetc(c, icontainer); ungetc('n', icontainer);
            ungetc('c', icontainer);
            buffer[i] = 'i'; i = clamp(++i);
            if (icns)
               fputc(buffer[i], icns);
         }
         }
         else { // reset
            ungetc(c, icontainer); ungetc('c', icontainer);
            buffer[i] = 'i'; i = clamp(++i);
            if (icns)
               fputc(buffer[i], icns);
         }
         }
         else { // reset
            ungetc(c, icontainer);
            buffer[i] = 'i'; i = clamp(++i);
            if (icns)
               fputc(buffer[i], icns);
         }
      }
   }
   fclose(icontainer);
   return 0;
}
