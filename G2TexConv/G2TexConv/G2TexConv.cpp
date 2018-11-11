// G2TexConv
// Originally written by Cippyboy of Xentax Forum, and called "Hitman_2016"

/*
	Refractoring done by HHCHunter
		- Removed 3D Model related functions, since they are not needed for this simple texture conversion tool.
		- Written comments for preparation to include updated detection, for including support for "HITMAN 2" textures.
*/

//Include Windows specific headers.
#include "stdafx.h"
#include <string>
#include <stdio.h>
#include <windows.h>

//Include program specific headers.
#include "G2TexConv.h"

//PROGRAM START

//GET THE LIST OF FILES IN THE FOLDER
void GetFilesInFolder(DYNAMIC_ARRAY < RE_STRING * > & List,
	const char * Folder, char * Filter) {
	if (!Folder || !Folder[0] || !Filter || !Filter[0])
		return;
	char FinalName[200] = "";
	sprintf(FinalName, "%s%s", Folder, Filter);

	WIN32_FIND_DATAA WinFile;
	ZeroMemory(&WinFile, sizeof(WinFile));
	//Can't handle multiple extension filters, weird
	HANDLE Handle = FindFirstFileA(FinalName, &WinFile);

	//Adds the first file in list
	if (WinFile.cFileName[0]) {
		RE_STRING * S = new RE_STRING;
		List.Add(S);
		(*S) += Folder;
		(*S) += WinFile.cFileName;
	}

	//Finds the next file in the list.
	while (FindNextFileA(Handle, &WinFile)) {
		RE_STRING * S = new RE_STRING;
		List.Add(S);
		(*S) += Folder;
		(*S) += WinFile.cFileName;
	}
	FindClose(Handle);
}

//GetMipSize from G2TexConv.h
int GetMipSize(TEXTURE_INFO Info) {
	int x = Info.Width;
	int y = Info.Height;
	int nBlockSize = 16;

	if (Info.Format == TEXTURE::DXT1 || Info.Format == TEXTURE::BC4) {
		nBlockSize = 8;
	}
	int MipSize = ((x + 3) / 4) * ((y + 3) / 4) * nBlockSize;
	return MipSize;
}

//Saves a DDS File
//May need to rewrite this section to include HeaderDAT
void SaveDDS(VIRTUAL_FILE * SourceFile, HeaderVAP * VAPHeader, int DataOffset, const char * FileName, TEXTURE::FORMAT Format) {
	//VAPHeader->Width /= 2;
	//VAPHeader->Height /= 2;

	uint32_t DDSmagic = 0x20534444;
	TEXTURE_INFO Info;
	Info.Format = Format; // TEXTURE::DXT3;
	Info.Width = VAPHeader - > Width;
	Info.Height = VAPHeader - > Height;
	int MipSize = GetMipSize(Info);

	if (MipSize > SourceFile - > Buffer.Size) {
		Info.Format = TEXTURE::DXT1;
		MipSize = GetMipSize(Info);
		//ExtraOffset = 0;
	}

	DDSURFACEDESC2 DDSHeader;
	memset(&DDSHeader, 0, sizeof(DDSHeader));
	DDSHeader.dwSize = 124;
	DDSHeader.dwFlags = DDSCAPS_TEXTURE; // 659463;
	DDSHeader.dwHeight = VAPHeader - > Height;
	DDSHeader.dwWidth = VAPHeader - > Width;
	DDSHeader.dwPitchOrLinearSize = MipSize;
	DDSHeader.dwDepth = 0;
	DDSHeader.dwMipMapCount = 1;
	//memset( DDSHeader.dwReserved1, 0, sizeof( DDSHeader.dwReserved1 ) );
	DDSHeader.dwReserved2 = 0;
	//memset( &DDSHeader.ddpfPixelFormat, 0, sizeof( DDSHeader.ddpfPixelFormat ) );

	DDSHeader.ddpfPixelFormat.dwSize = 32;
	DDSHeader.ddpfPixelFormat.dwFlags = 4;

	switch (Info.Format) {
	case TEXTURE::DXT1:
		DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT1;
		break;
	case TEXTURE::DXT2:
		DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT2;
		break;
	case TEXTURE::DXT3:
		DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT3;
		break;
	case TEXTURE::DXT4:
		DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT4;
		break;
	case TEXTURE::DXT5:
		DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
		break;

	case TEXTURE::BC4:
		DDSHeader.ddpfPixelFormat.dwFourCC = MAKEFOURCC('B', 'C', '4', 'U');
		break;
	case TEXTURE::BC5:
		DDSHeader.ddpfPixelFormat.dwFourCC = MAKEFOURCC('B', 'C', '5', 'U');
		break;

	case TEXTURE::BC6U:
	case TEXTURE::BC6S:
	case TEXTURE::BC7:
		DDSHeader.ddpfPixelFormat.dwFourCC = 808540228;
		break;
	}

	DDSHeader.ddsCaps.dwCaps1 = 0; // 4198408;
	DDSHeader.ddsCaps.dwCaps2 = 0;

	int ExtraHeader = 0;
	if (DDSHeader.ddpfPixelFormat.dwFourCC == 808540228) //DX10
	{
		ExtraHeader += sizeof(DX10Header);
	}
	int DDSDataOffset = sizeof(DDSmagic) + sizeof(DDSURFACEDESC2) + ExtraHeader;
	int DDSFileSize = MipSize + DDSDataOffset;
	BYTE * DDSFileData = new BYTE[DDSFileSize];
	memcpy(DDSFileData, &DDSmagic, 4);
	memcpy(DDSFileData + 4, &DDSHeader, sizeof(DDSHeader));
	if (DDSHeader.ddpfPixelFormat.dwFourCC == 808540228) {
		BYTE BC6Fix1 = 7;
		memcpy(DDSFileData + 8, &BC6Fix1, 1);
		BYTE BC6Fix2 = 0xA;
		memcpy(DDSFileData + 10, &BC6Fix2, 1);
		BYTE BC6Fix3 = 1;
		memcpy(DDSFileData + 0x18, &BC6Fix3, 1);

		DX10Header TheDX10Header;
		TheDX10Header.SetFormat(Info.Format);
		memcpy(DDSFileData + 4 + sizeof(DDSHeader), &TheDX10Header, sizeof(TheDX10Header));
	}
	SourceFile - > Offset = DataOffset;
	SourceFile - > Load(DDSFileData + DDSDataOffset, MipSize);

	SaveFile(FileName, DDSFileData, DDSFileSize);
}

//Extracts Textures from the user inputted predefined "Infolder" directory to the user inputted "Outfolder" directory.
void ExtractTextures(const char * InFolder, const char * OutFolder) {
	std::string InFolder2 = InFolder;
	InFolder2 += "\\";

	DYNAMIC_ARRAY < RE_STRING * > TextureList;
	//char * InFolder = "E:\\Downloads\\Downloaded Torrents\\GMT-MAX.ORG_Hitman_2016_Steam-Rip\\Hitman\\Runtime\\Extract\\TEXD\\";
	//char * InFolder = "C:\\Hitman_2016_Steam-Rip\\\Hitman\\Runtime\\Extract\\TEXD\\";

	//Gets all files that have the extension ".vap", needs to also support ".dat" files, as they are also texture files as well.
	GetFilesInFolder(TextureList, InFolder2.c_str(), "*.vap");

	//VIRTUAL_FILE TexFiles[ NumTexturesToLoad ];
	//HeaderVAP Headers[ NumTexturesToLoad ];
	for (int i = 0; i < TextureList.Size; i++) {
		const char * FileName = *TextureList[i];
		VIRTUAL_FILE * SourceFile = new VIRTUAL_FILE; // = &TexFiles[ i ];
		SourceFile - > LoadInMemory(FileName);
		int HeaderSize = sizeof(HeaderVAP);
		HeaderVAP * VAPHeader = new HeaderVAP; // &Headers[ i ];
		SourceFile - > Load((BYTE *)VAPHeader, HeaderSize);

		char FileWithPath[255];
		strcpy(FileWithPath, FileName);

		GetFileNameFromPath(FileWithPath);

		int DataOffset = SourceFile - > Offset;
		//clr
		printf("Converting %s ( %d / %d )\n", FileName, i, TextureList.Size);

		TEXTURE::FORMAT AttemptedFormat = TEXTURE::NO_FORMAT;
		if (VAPHeader - > Format == 0x49)
			AttemptedFormat = TEXTURE::DXT1;
		else if (VAPHeader - > Format == 79)
			AttemptedFormat = TEXTURE::DXT5;
		else if (VAPHeader - > Format == 82)
			AttemptedFormat = TEXTURE::BC4;
		else if (VAPHeader - > Format == 0x55)
			AttemptedFormat = TEXTURE::BC5;
		else if (VAPHeader - > Format == 0 || VAPHeader - > Format == 90)
			AttemptedFormat = TEXTURE::BC7;
		else if (VAPHeader - > Format == 28 || VAPHeader - > Format == 52) //No idea ?
			continue;
		//52 may be BC4U but not quite ! ( maybe BC4S ?)

		if (AttemptedFormat == TEXTURE::NO_FORMAT) {
			printf("Unknown Format %d\n", VAPHeader - > Format);
			continue;
		}
		//else
		//if ( AttemptedFormat == TEXTURE::NO_FORMAT )
		{
			/*TEXTURE::FORMAT PossibleFormats[] =
			{
			TEXTURE::DXT1,
			TEXTURE::DXT2,
			TEXTURE::DXT3,
			TEXTURE::DXT4,
			TEXTURE::DXT5,
			TEXTURE::BC4,
			TEXTURE::BC5,
			TEXTURE::BC6U,
			TEXTURE::BC6S,
			TEXTURE::BC7
			};
			int NumPossibleFormats = sizeof( PossibleFormats ) / sizeof( PossibleFormats[ 0 ] );
			*/
			//for ( int i = 0; i < NumPossibleFormats; i++ )
			{
				//AttemptedFormat = PossibleFormats[ i ];

				//std::string OutFolder = InFolder;
				//OutFolder += "ToDDS\\";
				CreateDirectoryA(OutFolder, 0);
				std::string FileOutName = OutFolder;
				FileOutName += "\\";
				FileOutName += FileWithPath;
				//FileOutName += std::to_string( u );
				const char * FormatAttempted = GetTextureFormatString(AttemptedFormat);
				FileOutName += ".";
				FileOutName += FormatAttempted;
				FileOutName += ".dds";

				//int ExtraOffset = u;

				SaveDDS(SourceFile, VAPHeader, DataOffset, FileOutName.c_str(), AttemptedFormat);
			}
			//break;
		}
	}
}

template < class T >
int64_t MaxElement(DYNAMIC_ARRAY < T > & Array) {
	T MaxValue = 0;
	int64_t MaxIndex = -1;
	for (int64_t i = 0; i < Array.Size; i++) {
		if (MaxIndex == -1 || Array[i] > MaxValue) {
			MaxValue = Array[i];
			MaxIndex = i;
		}
	}
	return MaxValue;
}

struct INT16_3 {
	int16_t x;
	int16_t y;
	int16_t z;
};

int main(int argc, const char ** arg) {
	if (argc != 3) {
		printf("Must give 2 paramaters ! first is input folder (Usually a TEXD Directory), second is output folder\n");
		printf("Usage\nHitman_2016.exe input_folder output_folder\n");
		printf("Example\nHitman_2016.exe Extract\\TEXD Extract\\TEXD\\Out\n");
		//return -1;
	}
	//ExtractTextures(arg[ 1 ], arg[ 2 ] );
	return 0;
}
