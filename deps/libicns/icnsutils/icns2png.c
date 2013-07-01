/*
File:       icnsinfo.c
Copyright (C) 2001-2012 Mathew Eis <mathew@eisbox.net>

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
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <png.h>

#include <icns.h>

typedef struct pixel32_t
{
	uint8_t	 r;
	uint8_t	 g;
	uint8_t	 b;
	uint8_t	 a;
} pixel32_t;

#define CONVERSION_SUCCESS   0  // Return code on success
#define CONVERSION_SHOWDOC   1  // Return code on --version/--help
#define CONVERSION_INVALID   2  // Return code on invalid arguments
#define CONVERSION_FAILURE   3  // Return code on conversion failure

#define	ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define	MAX_INPUTFILES  4096

#define	PRINT_ICNS_ERRORS	 1

int ExtractAndDescribeIconFamilyFile(char *filepath);
int ExtractAndDescribeIconFamily(icns_family_t *iconFamily,char *description,char *outfileprefix);
int WritePNGImage(FILE *outputfile,icns_image_t *image,icns_image_t *mask);

char 	*inputFileNames[MAX_INPUTFILES];
int	fileCount = 0;

/* Whether to list or extract icons */
#define	LIST_MODE	0x0001
#define	EXTRACT_MODE	0x0002
int	extractMode = 0;

/* Size and depth to match when extracting icons */
#define	MINI_SIZE	15 // 16x12 mini icon
#define	ALL_SIZES	0
#define	ALL_DEPTHS	0
int	extractIconSize = ALL_SIZES;
int	extractIconDepth = ALL_DEPTHS;

/* Optional output directory */
char    *outputPath = NULL;

const char *sizeStrs[] =  { "1024", "1024x1024" "512", "512x512", "256", "256x256", "128", "128x128", "48", "48x48", "32", "32x32", "16", "16x16", "16x12"    };
const int   sizeVals[] =  {  1024,   1024,       512,   512,       256,   256,       128,   128,       48,   48,      32,   32,      16,   16,      MINI_SIZE };

const char *depthStrs[] = { "32", "8", "4", "1" };
const int   depthVals[] = {  32 ,  8 ,  4 ,  1  };

int ParseSize(char *size)
{
	int i;
	int value = -1;
	
	if(size == NULL)
		return -1;
	
	for(i = 0; i < ARRAY_SIZE(sizeStrs); i++) {
		if(strcmp(sizeStrs[i], size) == 0) {
			value = sizeVals[i];
			break;
		}
	}
	
	return value;
}

int ParseDepth(char *depth)
{
	int i;
	int value = -1;
	
	if(depth == NULL)
		return -1;
	
	for(i = 0; i < ARRAY_SIZE(depthStrs); i++) {
		if(strcmp(depthStrs[i], depth) == 0) {
			value = depthVals[i];
			break;
		}
	}
	
	return value;
}

static void PrintVersionInfo(void)
{
	printf("icns2png 1.5                                                                  \n");
	printf("                                                                              \n");
	printf("Copyright (c) 2001-2012 Mathew Eis                                            \n");
	printf("This is free software; see the source for copying conditions.  There is NO    \n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   \n");
	printf("                                                                              \n");
	printf("Written by Mathew Eis                                                         \n");
}

static void PrintUsage(void)
{
	printf("Usage: icns2png [-x|-l] [options] [file ... ]                                 \n");
}

static void PrintHelp(void)
{
	printf("icns2png extracts images from mac .icns files, and exports them to png format.\n");
	printf("                                                                              \n");
	printf("Examples:                                                                     \n");
	printf("icns2png -x anicon.icns            # Extract all icon found in anicon.icns    \n");
	printf("icns2png -x -s 48 anicon.icns      # Extract all 48x48 32-bit icons           \n");
	printf("icns2png -x -s 32 -d 1 anicon.icns # Extract all 32x32 1-bit icons            \n");
	printf("icns2png -l anicon.icns            # Lists the icons contained in anicon.icns \n");
	printf("                                                                              \n");
	printf("Options:                                                                      \n");
	printf(" -l, --list    List the contents of one or more icns images                   \n");
	printf(" -x, --extract Extract one or more icons to png images                        \n");
	printf(" -o, --output  Where to place extracted files. If not specified, icons will be\n");
	printf("               extracted to the same path as the source file.                 \n");
	printf(" -d, --depth   Sets the pixel depth of the icons to extract. (1,4,8,32)       \n");
	printf(" -s, --size    Sets the width and height of the icons to extract. (16,48,etc) \n");
	printf("               Sizes 16x12, 16x16, 32x32, 48x48, 128x128, etc. are also valid.\n");
	printf(" -h, --help    Displays this help message.                                    \n");
	printf(" -v, --version Displays the version information                               \n");
}

static char *short_opts = "xlhvo:d:s:";
static struct option long_opts[] = {
	{ "list",     no_argument,        NULL, 'l' },
	{ "extract",  no_argument,        NULL, 'x' },
	{ "output",   required_argument,  NULL, 'o' },
	{ "depth",    required_argument,  NULL, 'd' },
	{ "size",     required_argument,  NULL, 's' },
	{ "help",     no_argument,        NULL, 'h' },
	{ "version",  no_argument,        NULL, 'v' },
	{ 0,          0,                  0,     0  }
};

int ParseOptions(int argc, char** argv)
{
	int opt = 0;

	if(argc < 2)
	{
		PrintUsage();
		return CONVERSION_INVALID;
	}

	extractMode = 0;

	while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
	{
		switch (opt) {
		case 'x':
			extractMode |= EXTRACT_MODE;
			break;
		case 'l':
			extractMode |= LIST_MODE;
			break;
		case 'o':
			// Should check for a valid directory here....
			outputPath = optarg;
			break;
		case 'd':
			extractIconDepth = ParseDepth(optarg);
			if(extractIconSize == -1) {
				fprintf(stderr, "Invalid icon color depth specified.\n");
				return CONVERSION_INVALID;
			}
			break;
		case 's':
			extractIconSize = ParseSize(optarg);
			if(extractIconSize == -1) {
				fprintf(stderr, "Invalid icon size specified.\n");
				return CONVERSION_INVALID;
			}
			break;
		case 'v':
			PrintVersionInfo();
			return CONVERSION_SHOWDOC;
		case 'h':
			PrintUsage();
			PrintHelp();
			return CONVERSION_SHOWDOC;
		case '?':
			return CONVERSION_SHOWDOC;
		}
	}

	if(extractMode == 0)
	{
		fprintf(stderr, "Must specify whether to list or extract icons.\n");
		PrintUsage();
		return CONVERSION_INVALID;
	}
	
	argc -= optind;
	argv += optind;
	
	while (argc) {
		if(fileCount >= MAX_INPUTFILES) {
			fprintf(stderr, "No more files can be added\n");
			break;
		}
		inputFileNames[fileCount] = malloc(strlen(argv[0])+1);
		if(!inputFileNames[fileCount]) {
			printf("Out of Memory\n");
			exit(CONVERSION_FAILURE);
		}
		strcpy(inputFileNames[fileCount], argv[0]);
		fileCount++;
		argc--;
		argv++;
	}
	
	return CONVERSION_SUCCESS;
}


int main(int argc, char *argv[])
{
	int result = CONVERSION_SUCCESS;
	int count = 0;
	
	// Initialize the globals;
	extractIconSize = ALL_SIZES;
	extractIconDepth = ALL_DEPTHS;
	extractMode = 0; // Do nothing
	
	// error messages handled by ParseOptions
    result = ParseOptions(argc, argv);
	if(result != CONVERSION_SUCCESS)
		return result;
	
	// display any exceptions thrown by libicns
	icns_set_print_errors(PRINT_ICNS_ERRORS);
	
	for(count = 0; count < fileCount; count++)
	{
        int convresult = ExtractAndDescribeIconFamilyFile(inputFileNames[count]);
		if(convresult != ICNS_STATUS_OK) {
			fprintf(stderr, "Errors while extracting icns data from %s!\n",inputFileNames[count]);
            if(result == CONVERSION_SUCCESS)
                result = CONVERSION_FAILURE;
        }
	}
	
	for(count = 0; count < fileCount; count++)
		if(inputFileNames[count] != NULL)
			free(inputFileNames[count]);

	return result;
}

int ExtractAndDescribeIconFamilyFile(char *filepath)
{
	int           error = ICNS_STATUS_OK;
	FILE          *inFile = NULL;
	icns_family_t *iconFamily = NULL;
	unsigned int  filepathlength = 0;
	char          *filename = NULL;
	unsigned int  filenamelength = 0;
	unsigned int  filenamestart = 0;
	char          *outfileprefix = NULL;
	unsigned int  outfileprefixlength = 0;
	
	#ifdef __APPLE__
	char          *rsrcfilepath = NULL;
	unsigned int  rsrcfilepathlength = 0;
	#endif
	
	filepathlength = strlen(filepath);
	
	#ifdef __APPLE__
	rsrcfilepathlength = filepathlength + 17;
	rsrcfilepath = (char *)malloc(rsrcfilepathlength);
	if(rsrcfilepath == NULL)
		return ICNS_STATUS_NO_MEMORY;
	
	// Append the OS X POSIX-safe filepath to access the resource fork
	strncpy(&rsrcfilepath[0],&filepath[0],filepathlength);
	strncpy(&rsrcfilepath[filepathlength],"/..namedfork/rsrc",17);
	#endif
	
	// Get the plain filename...
	filename = (char *)malloc(filepathlength + 1);
	filenamestart = filepathlength-1;
	while(filenamestart > 0) {
		if(filepath[filenamestart] == '/')
			break;
		filenamestart--;
	}
	filenamelength = filepathlength-filenamestart;
	memcpy(&filename[0],&filepath[filenamestart],filenamelength);
	filename[filenamelength] = 0;
	
	// Set up the output filepath...
	unsigned int	outputpathlength = 0;
	unsigned int	filepathstart = filepathlength;
	unsigned int	filepathend = filepathlength;

	if(outputPath != NULL)
	{	
		outputpathlength = strlen(outputPath);

		// Create a buffer large enough to hold the worst case
		outfileprefix = (char *)malloc(outputpathlength+filepathlength+1);
		if(outfileprefix == NULL)
		{
			error = ICNS_STATUS_NO_MEMORY;
			goto cleanup;
		}
		
		// Copy in the output path
		strncpy(&outfileprefix[0],&outputPath[0],outputpathlength);
		outfileprefixlength = outputpathlength;
		if(outputPath[outfileprefixlength-1] != '/')
		{
			outfileprefix[outfileprefixlength] = '/';
			outfileprefixlength++;
			outfileprefix[outfileprefixlength] = 0;
		}
		
		// Append the output filepath
		// That is, the input filepath from the last '/' or start to the last '.' past the last '/'
		while(filepath[filepathstart] != '/' && filepathstart > 0)
			filepathstart--;
		if(filepathstart != 0)
			filepathstart++;
		while(filepath[filepathend] != '.' && filepathend > filepathstart)
			filepathend--;
		if(filepathend == filepathstart)
			filepathend = filepathlength;
		strncpy(&outfileprefix[outfileprefixlength],&filepath[filepathstart],filepathend - filepathstart);
		outfileprefixlength += (filepathend - filepathstart);
		outfileprefix[outfileprefixlength] = 0;
	}
	else
	{
		// Create a buffer for the input filepath (without the new extension)
		outfileprefix = (char *)malloc(filepathlength+1);
		if(outfileprefix == NULL)
			return ICNS_STATUS_NO_MEMORY;

		// Create the output filepath
		// That is, the input filepath from the last '/' or start to the last '.' past the last '/'
		while(filepath[filepathstart] != '/' && filepathstart > 0)
			filepathstart--;
		if(filepathstart != 0)
			filepathstart++;
		while(filepath[filepathend] != '.' && filepathend > filepathstart)
			filepathend--;
		if(filepathend == filepathstart)
			filepathend = filepathlength;
		strncpy(&outfileprefix[0],&filepath[filepathstart],filepathend - filepathstart);
		outfileprefixlength += (filepathend - filepathstart);
		outfileprefix[outfileprefixlength] = 0;
	}
	
	printf("----------------------------------------------------\n");
	printf("Reading icns family from %s...\n",filepath);
	
	#ifdef __APPLE__
	// If we're on an apple system, we want to try
	// reading the resource fork first...
	
	// hide all internal errors while we try to read the resource fork
	icns_set_print_errors(0);

	inFile = fopen( rsrcfilepath, "r" );
	
	if ( inFile != NULL ) {
		error = icns_read_family_from_rsrc(inFile,&iconFamily);
		if(error == ICNS_STATUS_OK)
			printf("Using icon from HFS+ resource fork...\n");
		fclose(inFile);
		inFile = NULL;
	} else {
		error = ICNS_STATUS_IO_READ_ERR;
	}
	
	// Turn them back on if we wanted them
	icns_set_print_errors(PRINT_ICNS_ERRORS);
	
	// If we had an error, it was from trying to read the resource fork, so try the data file
	if(error != ICNS_STATUS_OK)
	{
		inFile = fopen( filepath, "r" );
		
		if ( inFile == NULL ) {
			fprintf (stderr, "Unable to open file %s!\n",filepath);
			goto cleanup;
		}

		error = icns_read_family_from_file(inFile,&iconFamily);
		
		fclose(inFile);
	}
	
	#else
	// On all other systems, read just the file

	inFile = fopen( filepath, "r" );
	
	if ( inFile == NULL ) {
		fprintf (stderr, "Unable to open file %s!\n",filepath);
		goto cleanup;
	}

	error = icns_read_family_from_file(inFile,&iconFamily);
		
	fclose(inFile);

	#endif
			
	if(error) {
		fprintf (stderr, "Unable to read icns family from file %s!\n",filepath);
		goto cleanup;
	}
	
	error = ExtractAndDescribeIconFamily(iconFamily,filename,outfileprefix);

cleanup:
	
	
	#ifdef __APPLE__
	if(rsrcfilepath != NULL) {
		free(rsrcfilepath);
		rsrcfilepath = NULL;
	}
	#endif
	if(iconFamily != NULL) {
		free(iconFamily);
		iconFamily = NULL;
	}
	if(outfileprefix != NULL) {
		free(outfileprefix);
		outfileprefix = NULL;
	}
	if(filename != NULL) {
		free(filename);
		filename = NULL;
	}

	
	return error;
}

int ExtractAndDescribeIconFamily(icns_family_t *iconFamily,char *description,char *outfileprefix) {
	int		error = ICNS_STATUS_OK;
	icns_byte_t *dataPtr = (icns_byte_t*)iconFamily;
	unsigned long  dataOffset = 0;
	int           imageCount = 0;
	int           elementCount = 0;
	int           extractedCount = 0;
	char           *outfilepath = NULL;
	
	printf(" Extracting icons from %s...\n",description);
	
	// Create a buffer for the output filename
	if(extractMode & EXTRACT_MODE) {
		outfilepath = (char *)malloc(strlen(outfileprefix)+25);
		if(outfilepath == NULL)
			return ICNS_STATUS_NO_MEMORY;
	}
	
	// Start listing info:
	if(extractMode & LIST_MODE) {
		char typeStr[5];
		icns_type_str(iconFamily->resourceType,typeStr);
		printf(" Icon family size is %d bytes (including %d byte header)\n",iconFamily->resourceSize,8);
	}
	
	// Skip past the icns header
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	dataPtr = (icns_byte_t *)iconFamily;
	
	if(extractMode & LIST_MODE)
		printf(" Listing icon elements...\n");
	
    // Loop through and convert each icon
	while(((dataOffset+8) < iconFamily->resourceSize) && (error == 0 || error == ICNS_STATUS_UNSUPPORTED))
	{
		icns_element_t	 iconElement;
		icns_icon_info_t iconInfo;
		icns_size_t      iconDataSize;
		icns_size_t      iconDimSize = 0;
		char	         typeStr[5];
		
		memcpy(&iconElement,(dataPtr+dataOffset),8);
		
		icns_type_str(iconElement.elementType,typeStr);
		iconDataSize = iconElement.elementSize - 8;
		
		if(extractMode & LIST_MODE) {
			printf("  '%s'",typeStr);
		}
		
		switch(iconElement.elementType) {
			case ICNS_TABLE_OF_CONTENTS:
			{
				if(extractMode & LIST_MODE) {
					printf(" table of contents\n");
				}
			}
			break;
			case ICNS_ICON_VERSION:
			{
				icns_byte_t	iconBytes[4];
				icns_uint32_t	iconVersion = 0;
				float		iconVersionNumber = 0;
				if(iconDataSize == 4) {
					memcpy(&iconBytes[0],(dataPtr+dataOffset+8),4);
					iconVersion = iconBytes[3]|iconBytes[2]<<8|iconBytes[1]<<16|iconBytes[0]<<24;
					iconVersionNumber = *((float *)(&iconVersion));
				}
				if(extractMode & LIST_MODE) {
					printf(" value: %f\n",iconVersionNumber);
				}
			}
			break;
			case ICNS_TILE_VARIANT:
			case ICNS_ROLLOVER_VARIANT:
			case ICNS_DROP_VARIANT:
			case ICNS_OPEN_VARIANT:
			case ICNS_OPEN_DROP_VARIANT:
			{
				// Load up the variant, over-write the type and size to make it into an icns
				icns_byte_t	*variantData = (icns_byte_t*)malloc(iconElement.elementSize);
				icns_family_t *variant = NULL;
				icns_byte_t	b[4] = {0,0,0,0};
				memcpy(variantData,(dataPtr+dataOffset),iconElement.elementSize);
				memcpy(variantData,"icns",4);
				b[0] = iconElement.elementSize >> 24;
				b[1] = iconElement.elementSize >> 16;
				b[2] = iconElement.elementSize >> 8;
				b[3] = iconElement.elementSize;
				memcpy(&variantData[4], &b[0], sizeof(icns_size_t));
				
				// Display some info about the variant
				switch(iconElement.elementType) {
					case ICNS_TILE_VARIANT:
						if(extractMode & LIST_MODE)
							printf(" icon variant: tile (%d bytes)\n",iconDataSize);
						break;
					case ICNS_ROLLOVER_VARIANT:
						if(extractMode & LIST_MODE)
							printf(" icon variant: rollover (%d bytes)\n",iconDataSize);
						break;
					case ICNS_DROP_VARIANT:
						if(extractMode & LIST_MODE)
							printf(" icon variant: drop (%d bytes)\n",iconDataSize);
						break;
					case ICNS_OPEN_VARIANT:
						if(extractMode & LIST_MODE)
							printf(" icon variant: open (%d bytes)\n",iconDataSize);
						break;
					case ICNS_OPEN_DROP_VARIANT:
						if(extractMode & LIST_MODE)
							printf(" icon variant: open/drop (%d bytes)\n",iconDataSize);
						break;
				}

				// Try to parse out the variant
				error = icns_import_family_data(iconElement.elementSize,variantData,&variant);
				
				if(error) {
					fprintf (stderr, "Unable to read icon variant type '%s' (error while parsing)\n",typeStr);
				} else {
					icns_size_t	variantLength = strlen(outfileprefix) + strlen(typeStr) + 2;
					char *variantPrefix = (char *)malloc(variantLength);
					if(variantPrefix != NULL) {
						sprintf(&variantPrefix[0],"%s_%s",outfileprefix,typeStr);
						variantPrefix[variantLength] = 0;
						error = ExtractAndDescribeIconFamily((icns_family_t*)variant,typeStr,variantPrefix);
						free(variantPrefix);
					}
				}
				
				free(variant);
				
				if(variantData) {
					free(variantData);
					variantData = NULL;
				}
			}
			break;
			default:
            {
				iconInfo = icns_get_image_info_for_type(iconElement.elementType);
				
				if(iconInfo.iconWidth == iconInfo.iconHeight) {
					iconDimSize = iconInfo.iconWidth;
				} else if(iconElement.elementType == ICNS_16x12_8BIT_DATA) {
					iconDimSize = MINI_SIZE;
				} else if(iconElement.elementType == ICNS_16x12_4BIT_DATA) {
					iconDimSize = MINI_SIZE;
				} else if(iconElement.elementType == ICNS_16x12_1BIT_DATA) {
					iconDimSize = MINI_SIZE;
				} else if(iconElement.elementType == ICNS_16x12_1BIT_MASK) {
					iconDimSize = MINI_SIZE;
				} else {
					iconDimSize = -1;
				}
                
                if(iconInfo.isImage)
                    imageCount++;
				
				if(extractMode & LIST_MODE)
				{
					// size
					printf(" %dx%d",iconInfo.iconWidth,iconInfo.iconHeight);
					// bit depth
					printf(" %d-bit",iconInfo.iconBitDepth);
					if(iconInfo.isImage)
						printf(" icon");
					if(iconInfo.isImage && iconInfo.isMask)
						printf(" with");
					if(iconInfo.isMask)
						printf(" mask");
					if((iconElement.elementSize-8) < iconInfo.iconRawDataSize) {
						printf(" (%d bytes compressed to %d)",(int)iconInfo.iconRawDataSize,iconDataSize);
					} else {
						printf(" (%d bytes)",iconDataSize);
					}
					printf("\n");
				}
				
				if(extractMode & EXTRACT_MODE)
				{
				if(extractIconSize == ALL_SIZES || extractIconSize == iconDimSize)
				{
				if(extractIconDepth == ALL_DEPTHS || extractIconDepth == iconInfo.iconBitDepth)
				{
				if(iconInfo.isImage)
				{
					unsigned int	outfilepathlength = 0;
					FILE 		*outfile = NULL;
					icns_image_t	iconImage;
					
					memset ( &iconImage, 0, sizeof(icns_image_t) );
					
					error = icns_get_image32_with_mask_from_family(iconFamily,iconElement.elementType,&iconImage);
                    
                    if(error == ICNS_STATUS_UNSUPPORTED)
                    {
                        printf("  Unable to convert '%s' element! (Unsupported by this version of libicns)\n",typeStr);
                    }
                    else if(error != ICNS_STATUS_OK)
					{
						fprintf (stderr, "Unable to load 32-bit icon image with mask from icon family!\n");
					}
					else
					{
						// Set up the output file name: description_WWxHHxDD.png
						outfilepathlength = sprintf(&outfilepath[0],"%s_%dx%dx%d.png",outfileprefix,iconInfo.iconWidth,iconInfo.iconHeight,iconInfo.iconBitDepth);
						outfilepath[outfilepathlength] = 0;
						
						outfile = fopen(outfilepath,"w");
						if(!outfile)
						{
							fprintf (stderr, "Unable to open %s for writing!\n",outfilepath);
						}
						else
						{
							error = WritePNGImage(outfile,&iconImage,NULL);
							
							if(error) {
								fprintf (stderr, "Error writing PNG image!\n");
							} else {
								printf("  Saved '%s' element to %s.\n",typeStr,outfilepath);
							}
							
							if(outfile != NULL) {
								fclose(outfile);
								outfile = NULL;
							}
						}
						
						extractedCount++;
						
						icns_free_image(&iconImage);
					}
				}
				}
				}
				}
            }
            break;
		}
		
		// Move on to the next element
		dataOffset += iconElement.elementSize;
		elementCount++;
	}
	
	if(extractMode & LIST_MODE)
	{
		if(elementCount > 0) {
			printf("%d elements total found in %s.\n",elementCount,description);
		} else {
			printf("No elements found in %s.\n",description);
		}
	}
	
	if(extractMode & EXTRACT_MODE)
	{
		if(extractedCount > 0) {
            if(extractedCount == imageCount) {
                printf("Extracted %d images from %s.\n",extractedCount,description);
            } else {
                printf("Extracted %d of %d images from %s.\n",extractedCount,imageCount,description);
            }
		} else {
			printf("No elements were extracted from %s.\n",description);
		}		
	}
	
	if(outfilepath != NULL) {
		free(outfilepath);
		outfilepath = NULL;
	}
	
	return error;
}

//***************************** WritePNGImage **************************//
// Relatively generic PNG file writing routine

int	WritePNGImage(FILE *outputfile,icns_image_t *image,icns_image_t *mask)
{
	int 			width = 0;
	int 			height = 0;
	int 			image_channels = 0;
	int			image_pixel_depth = 0;
	int 			mask_channels = 0;
	png_structp 		png_ptr;
	png_infop 		info_ptr;
	png_bytep 		*row_pointers;
	int			i, j;
	
	if (image == NULL)
	{
		fprintf (stderr, "icns image NULL!\n");
		return -1;
	}
	
	width = image->imageWidth;
	height = image->imageHeight;
	image_channels = image->imageChannels;
	image_pixel_depth = image->imagePixelDepth;
	
	/*
	printf("width: %d\n",width);
	printf("height: %d\n",height);
	printf("image_channels: %d\n",image_channels);
	printf("image_pixel_depth: %d\n",image_pixel_depth);
	*/
	
	if(mask != NULL) {
		mask_channels = mask->imageChannels;
	}
	
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (png_ptr == NULL)
	{
		fprintf (stderr, "PNG error: cannot allocate libpng main struct\n");
		return -1;
	}

	info_ptr = png_create_info_struct (png_ptr);

	if (info_ptr == NULL)
	{
		fprintf (stderr, "PNG error: cannot allocate libpng info struct\n");
		png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
		return -1;
	}

	png_init_io(png_ptr, outputfile);

	png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
	
	png_set_IHDR (png_ptr, info_ptr, width, height, image_pixel_depth, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info (png_ptr, info_ptr);
	
	if(image_pixel_depth < 8)
		png_set_packing (png_ptr);
	
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep)*height);
	
	if (row_pointers == NULL)
	{
		fprintf (stderr, "PNG error: unable to allocate row_pointers\n");
	}
	else
	{
		for (i = 0; i < height; i++)
		{
			if ((row_pointers[i] = (png_bytep)malloc(width*image_channels)) == NULL)
			{
				fprintf (stderr, "PNG error: unable to allocate rows\n");
				for (j = 0; j < i; j++)
					free(row_pointers[j]);
				free(row_pointers);
				return -1;
			}
			
			for(j = 0; j < width; j++)
			{
				pixel32_t	*src_rgb_pixel;
				pixel32_t	*src_pha_pixel;
				pixel32_t	*dst_pixel;
				
				dst_pixel = (pixel32_t *)&(row_pointers[i][j*image_channels]);
				
				src_rgb_pixel = (pixel32_t *)&(image->imageData[i*width*image_channels+j*image_channels]);

				dst_pixel->r = src_rgb_pixel->r;
				dst_pixel->g = src_rgb_pixel->g;
				dst_pixel->b = src_rgb_pixel->b;
				
				if(mask != NULL) {
					src_pha_pixel = (pixel32_t *)&(mask->imageData[i*width*mask_channels+j*mask_channels]);
					dst_pixel->a = src_pha_pixel->a;
				} else {
					dst_pixel->a = src_rgb_pixel->a;
				}
			}
		}
	}
	
	png_write_image (png_ptr,row_pointers);
	
	png_write_end (png_ptr, info_ptr);
	
	png_destroy_write_struct (&png_ptr, &info_ptr);
	
	for (j = 0; j < height; j++)
		free(row_pointers[j]);
	free(row_pointers);

	return 0;
}


