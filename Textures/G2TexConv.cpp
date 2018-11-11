// Hitman_2016.cpp : Defines the entry point for the console application.
// Originally written by Cippyboy of Xentax Forum.
// Refractor done by HHCHunter to remove 3D Model support & update detection.

#include "stdafx.h"

#include <Windows.h>
#include "../../REFramework/C++/REFramework/REFramework.h"
#include "../../REFramework/C++/REFramework/Images.h"
#include "../../REFramework/C++/REFramework/RenderObject.h"

#ifdef _DEBUG
	#pragma comment(lib, "REFramework_d.lib")
#else
	#pragma comment(lib, "REFramework_r.lib")
#endif

UINT EnableVSync = 0;//RE stupid dependency

#include <map>
#include <vector>
#include <string>

#include "G2TexConv.h"

void GetFilesInFolder( DYNAMIC_ARRAY<RE_STRING*> & List, const char *Folder, char *Filter )
{
	if ( !Folder || !Folder[ 0 ] || !Filter || !Filter[ 0 ] )
		return;
	char FinalName[ 200 ] = "";
	sprintf( FinalName, "%s%s", Folder, Filter );

	WIN32_FIND_DATAA WinFile;
	ZeroMemory( &WinFile, sizeof( WinFile ) );
	//Can't handle multiple extension filters, weird
	HANDLE Handle = FindFirstFileA( FinalName, &WinFile );

	//Add the first file in list
	if ( WinFile.cFileName[ 0 ] )
	{
		RE_STRING *S = new RE_STRING;
		List.Add( S );
		( *S ) += Folder;
		( *S ) += WinFile.cFileName;
	}

	while ( FindNextFileA( Handle, &WinFile ) )
	{
		RE_STRING *S = new RE_STRING;
		List.Add( S );
		( *S ) += Folder;
		( *S ) += WinFile.cFileName;
	}
	FindClose( Handle );
}
// GetMipSize from G2TexConv.h
int GetMipSize( TEXTURE_INFO Info )
{
	int x = Info.Width;
	int y = Info.Height;
	int nBlockSize = 16;

	if ( Info.Format == TEXTURE::DXT1 || Info.Format == TEXTURE::BC4 )
	{
		nBlockSize = 8;
	}
	int MipSize = ( ( x + 3 ) / 4 ) * ( ( y + 3 ) / 4 ) * nBlockSize;
	return MipSize;
}

void SaveDDS( VIRTUAL_FILE *SourceFile, HeaderVAP *VAPHeader, int DataOffset, const char *FileName, TEXTURE::FORMAT Format )
{
	//VAPHeader->Width /= 2;
	//VAPHeader->Height /= 2;

	uint32_t DDSmagic = 0x20534444;
	TEXTURE_INFO Info;
	Info.Format = Format;// TEXTURE::DXT3;
	Info.Width = VAPHeader->Width;
	Info.Height = VAPHeader->Height;
	int MipSize = GetMipSize( Info );

	if ( MipSize > SourceFile->Buffer.Size )
	{
		Info.Format = TEXTURE::DXT1;
		MipSize = GetMipSize( Info );
		//ExtraOffset = 0;
	}

	DDSURFACEDESC2 DDSHeader;
	memset( &DDSHeader, 0, sizeof( DDSHeader ) );
	DDSHeader.dwSize = 124;
	DDSHeader.dwFlags = DDSCAPS_TEXTURE;// 659463;
	DDSHeader.dwHeight = VAPHeader->Height;
	DDSHeader.dwWidth = VAPHeader->Width;
	DDSHeader.dwPitchOrLinearSize = MipSize;
	DDSHeader.dwDepth = 0;
	DDSHeader.dwMipMapCount = 1;
	//memset( DDSHeader.dwReserved1, 0, sizeof( DDSHeader.dwReserved1 ) );
	DDSHeader.dwReserved2 = 0;
	//memset( &DDSHeader.ddpfPixelFormat, 0, sizeof( DDSHeader.ddpfPixelFormat ) );

	DDSHeader.ddpfPixelFormat.dwSize = 32;
	DDSHeader.ddpfPixelFormat.dwFlags = 4;

	switch ( Info.Format )
	{
		case TEXTURE::DXT1: DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT1; break;
		case TEXTURE::DXT2: DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT2; break;
		case TEXTURE::DXT3: DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT3; break;
		case TEXTURE::DXT4: DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT4; break;
		case TEXTURE::DXT5: DDSHeader.ddpfPixelFormat.dwFourCC = FOURCC_DXT5; break;

		case TEXTURE::BC4: DDSHeader.ddpfPixelFormat.dwFourCC = MAKEFOURCC( 'B', 'C', '4', 'U' ); break;
		case TEXTURE::BC5: DDSHeader.ddpfPixelFormat.dwFourCC = MAKEFOURCC( 'B', 'C', '5', 'U' ); break;

		case TEXTURE::BC6U:
		case TEXTURE::BC6S:
		case TEXTURE::BC7 : DDSHeader.ddpfPixelFormat.dwFourCC = 808540228; break;
	}
	
	DDSHeader.ddsCaps.dwCaps1 = 0;// 4198408;
	DDSHeader.ddsCaps.dwCaps2 = 0;

	int ExtraHeader = 0;
	if ( DDSHeader.ddpfPixelFormat.dwFourCC == 808540228 )//DX10
	{
		ExtraHeader += sizeof( DX10Header );
	}
	int DDSDataOffset = sizeof( DDSmagic ) + sizeof( DDSURFACEDESC2 ) + ExtraHeader;
	int DDSFileSize = MipSize + DDSDataOffset;
	BYTE *DDSFileData = new BYTE[ DDSFileSize ];
	memcpy( DDSFileData, &DDSmagic, 4 );
	memcpy( DDSFileData + 4, &DDSHeader, sizeof( DDSHeader ) );
	if ( DDSHeader.ddpfPixelFormat.dwFourCC == 808540228 )
	{
		BYTE BC6Fix1 = 7;
		memcpy( DDSFileData + 8, &BC6Fix1, 1 );
		BYTE BC6Fix2 = 0xA;
		memcpy( DDSFileData + 10, &BC6Fix2, 1 );
		BYTE BC6Fix3 = 1;
		memcpy( DDSFileData + 0x18, &BC6Fix3, 1 );

		DX10Header TheDX10Header;
		TheDX10Header.SetFormat( Info.Format );
		memcpy( DDSFileData + 4 + sizeof( DDSHeader ), &TheDX10Header, sizeof( TheDX10Header ) );		
	}
	SourceFile->Offset = DataOffset;
	SourceFile->Load( DDSFileData + DDSDataOffset, MipSize );

	SaveFile( FileName, DDSFileData, DDSFileSize );
}

void ExportOBJ( const char *Filename, HMModel *Model )
{
	FILE *f = fopen( Filename, "w" );
	if ( !f )
		return;

	//fprintf( f, "#Vertices %d\n", NumVerts );
	//fprintf( f, "#Indices %d\n", NumIndices );

	UINT *IndicesOffsets = new UINT[ Model->Objects.Size ];
	//UINT *IndicesOffsets = new UINT[ Model->Objects.Size ];

	memset( IndicesOffsets, 0, sizeof( UINT ) * Model->Objects.Size );

	//if ( 0 )
	for ( int i = 0; i < Model->Objects.Size; i++ )
	{
		HMObject *O = Model->Objects[ i ];

		for ( int u = 0; u < O->Vertices.Size;u++ )
		{
			Vector3 *V = &O->Vertices[ u ];
			fprintf( f, "v %f %f %f\n", V->x, V->y, V->z );
		}
		if ( i == 0 )
			IndicesOffsets[ i ] = 0;
		else
			IndicesOffsets[ i ] = IndicesOffsets[ i - 1 ] + Model->Objects[ i - 1 ]->Vertices.Size;
	}

	for ( int i = 0; i < Model->Objects.Size; i++ )
	{
		HMObject *O = Model->Objects[ i ];

		for ( int u = 0; u < O->Indices.Size; u+=3 )
		{
			WORD *Indices = O->Indices.Array;
			UINT Offset = IndicesOffsets[ i ];
			WORD I1 = Indices[ u + 0 ] + 1 + Offset;
			WORD I2 = Indices[ u + 1 ] + 1 + Offset;
			WORD I3 = Indices[ u + 2 ] + 1 + Offset;
			fprintf( f, "f %d %d %d\n", I1, I2, I3 );
		}

		//break;
	}

	fclose( f );
}
void ExportOBJ( const char *Filename, int NumVerts, Vector3 *Vertices, Vector2 *Texcoords, int NumIndices,
				WORD *Indices )
{
	FILE *f = fopen( Filename, "w" );
	if ( !f )
		return;

	fprintf( f, "#Vertices %d\n", NumVerts );
	fprintf( f, "#Indices %d\n", NumIndices );

	for ( int i = 0; i < NumVerts; i++ )
	{
		Vector3 *V = &Vertices[ i ];
		fprintf( f, "v %f %f %f\n", V->x, V->y, V->z );
	}

	//fprintf( f, "\n" );

	if ( 0 )// Texcoords )
	{
		for ( int i = 0; i < NumVerts; i++ )
		{
			Vector2 *VT = &Texcoords[ i ];
			fprintf( f, "vt %f %f\n", VT->x, VT->y );
		}
	}

	if ( 0 )
	{
		for ( int i = 0; i < NumVerts; i++ )
		{
			Vector3 *V = &Vertices[ i ];
			fprintf( f, "vn %f %f %f\n", V->x, V->y, V->z );
		}
	}
	if ( Indices )
	{
		for ( int i = 0; i < NumIndices; i+= 3 )//++ )
		{
			WORD I1 = Indices[ i + 0 ] + 1;
			WORD I2 = Indices[ i + 1 ] + 1;
			WORD I3 = Indices[ i + 2 ] + 1;
			fprintf( f, "f %d %d %d\n", I1, I2, I3 );
			//fprintf( f, "f %d/%d %d/%d %d/%d\n", I1, I1, I2, I2, I3, I3 );
			//fprintf( f, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", I1, I1, I1, I2, I2, I2, I3, I3, I3 );
		}
		/*for ( int i = 0; i < NumIndices; i += 4 )
		{
			WORD I1 = Indices[ i + 0 ];
			WORD I2 = Indices[ i + 1 ];
			WORD I3 = Indices[ i + 2 ];
			fprintf( f, "f %d %d %d\n", I1, I2, I3 );

			I1 = Indices[ i + 1 ];
			I2 = Indices[ i + 2 ];
			I3 = Indices[ i + 3 ];
			fprintf( f, "f %d %d %d\n", I1, I2, I3 );
		}*/
	}

	fclose( f );
}

void ExtractTextures( const char * InFolder, const char *OutFolder )
{
	std::string InFolder2 = InFolder;
	InFolder2 += "\\";

	DYNAMIC_ARRAY<RE_STRING*> TextureList;
	//char * InFolder = "E:\\Downloads\\Downloaded Torrents\\GMT-MAX.ORG_Hitman_2016_Steam-Rip\\Hitman\\Runtime\\Extract\\TEXD\\";
	//char * InFolder = "C:\\Hitman_2016_Steam-Rip\\\Hitman\\Runtime\\Extract\\TEXD\\";
	GetFilesInFolder( TextureList, InFolder2.c_str(), "*.vap" );

	//VIRTUAL_FILE TexFiles[ NumTexturesToLoad ];
	//HeaderVAP Headers[ NumTexturesToLoad ];
	for ( int i = 0; i < TextureList.Size; i++ )
	{
		const char * FileName = *TextureList[ i ];
		VIRTUAL_FILE *SourceFile = new VIRTUAL_FILE;// = &TexFiles[ i ];
		SourceFile->LoadInMemory( FileName );
		int HeaderSize = sizeof( HeaderVAP );
		HeaderVAP *VAPHeader = new HeaderVAP;// &Headers[ i ];
		SourceFile->Load( (BYTE*)VAPHeader, HeaderSize );

		char FileWithPath[ 255 ];
		strcpy( FileWithPath, FileName );

		GetFileNameFromPath( FileWithPath );

		int DataOffset = SourceFile->Offset;
		//clr
		printf( "Converting %s ( %d / %d )\n", FileName, i, TextureList.Size );

		TEXTURE::FORMAT AttemptedFormat = TEXTURE::NO_FORMAT;
		if ( VAPHeader->Format == 0x49 )
			AttemptedFormat = TEXTURE::DXT1;
		else if ( VAPHeader->Format == 79 )
			AttemptedFormat = TEXTURE::DXT5;
		else if ( VAPHeader->Format == 82 )
			AttemptedFormat = TEXTURE::BC4;
		else if ( VAPHeader->Format == 0x55 )
			AttemptedFormat = TEXTURE::BC5;
		else if ( VAPHeader->Format == 0 || VAPHeader->Format == 90 )
			AttemptedFormat = TEXTURE::BC7;
		else if ( VAPHeader->Format == 28 || VAPHeader->Format == 52 )//No idea ?
			continue;
		//52 may be BC4U but not quite ! ( maybe BC4S ?)

		if ( AttemptedFormat == TEXTURE::NO_FORMAT )
		{
			printf( "Unknown Format %d\n", VAPHeader->Format );
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
				CreateDirectoryA( OutFolder, 0 );
				std::string FileOutName = OutFolder;
				FileOutName += "\\";
				FileOutName += FileWithPath;
				//FileOutName += std::to_string( u );
				const char *FormatAttempted = GetTextureFormatString( AttemptedFormat );
				FileOutName += ".";
				FileOutName += FormatAttempted;
				FileOutName += ".dds";

				//int ExtraOffset = u;

				SaveDDS( SourceFile, VAPHeader, DataOffset, FileOutName.c_str(), AttemptedFormat );
			}
			//break;
		}
	}
}

template<class T>
int64_t MaxElement( DYNAMIC_ARRAY<T> & Array )
{
	T MaxValue = 0;
	int64_t MaxIndex = -1;
	for ( int64_t i = 0; i < Array.Size; i++ )
	{
		if ( MaxIndex == -1 || Array[ i ] > MaxValue )
		{
			MaxValue = Array[ i ];
			MaxIndex = i;
		}
	}
	return MaxValue;
}

void ExportObjectToREM_XML( const char *Filename, int NumVerts, Vector3 *Vertices, Vector2 *Texcoords, int NumIndices, WORD *Indices );
void ExportObjectToREM_XML( const char *Filename, HMModel *Model );

struct INT16_3
{ 
	int16_t x;
	int16_t y;
	int16_t z;
};
void ProcessObject( VIRTUAL_FILE & VF, HMMainBlock & MainBlock, HMModel *Model, HMBlock3 & Block3, int Object )
{
	HMObject *NewObject = new HMObject;
	NewObject->Name = std::to_string( MainBlock.NextOffset );
	Model->Objects.Add( NewObject );

	NewObject->Indices.Allocate( Block3.NumIndices );
	NewObject->Vertices.Allocate( Block3.NumVertices );
	DYNAMIC_ARRAY<uint16_t> *Indices = &NewObject->Indices;
	DYNAMIC_ARRAY<Vector3> *Vertices = &NewObject->Vertices;

	DYNAMIC_ARRAY<INT16_3> Vertices16bit( Block3.NumVertices, 4 );

	VF.Offset = Block3.IndicesOffset;
	for ( int i = 0; i < Block3.NumIndices; i++ )
	{
		uint16_t NewIndex = 0;
		VF.Load( (BYTE*)&NewIndex, 1 * sizeof( uint16_t ) );
		Indices->Add( NewIndex );
	}

	VF.Offset = Block3.VertexOffset;
	VF.Load( (BYTE*)Vertices->Array, Block3.NumVertices * sizeof( Vector3 ) );
	Vertices->Size = Block3.NumVertices;

	bool Use16bitVertices = true;

	if ( Use16bitVertices )
	{
		memcpy( Vertices16bit.Array, Vertices->Array, Block3.NumVertices * sizeof( INT16_3 ) );
		Vertices16bit.Size = Block3.NumVertices;

		for ( int i = 0; i < Block3.NumVertices; i++ )
		{
			( *Vertices )[ i ].x = (float)Vertices16bit[ i ].x / 32767.0f;
			( *Vertices )[ i ].y = (float)Vertices16bit[ i ].y / 32767.0f;
			( *Vertices )[ i ].z = (float)Vertices16bit[ i ].z / 32767.0f;
		}
	}

	bool Flip = true;
	if ( Flip )
	{
		for ( int i = 0; i < Vertices->Size; i++ )
		{
			float Temp = ( *Vertices )[ i ].y;
			(*Vertices)[ i ].y = ( *Vertices )[ i ].z;
			( *Vertices )[ i ].z = Temp;
		}
	}

	DYNAMIC_ARRAY<WORD2> Texcoords16( Block3.NumVertices, 4 );
	DYNAMIC_ARRAY<Vector2> Texcoords32( Block3.NumVertices, 4 );
	VF.Load( (BYTE*)Texcoords16.Array, Block3.NumVertices * sizeof( WORD2 ) );
	Texcoords16.Size = Block3.NumVertices;

	for ( int i = 0; i < Texcoords16.Size; i++ )
		Texcoords32.Add( Vector2( (float)Texcoords16[ i ].X / 65535.0f, (float)Texcoords16[ i ].Y / 65535.0f ) );

	//std::string Str = "Out\\Exported_";
	//Str += std::to_string( Object );
	//Str += ".xml";
	//ExportOBJ( Str.c_str(), Vertices.Size, Vertices.Array, Texcoords32.Array, Indices.Size, Indices.Array );

	//ExportObjectToREM_XML( Str.c_str(), Vertices.Size, Vertices.Array, Texcoords32.Array, Indices.Size, Indices.Array );
}
void ProcessMainBlock( HMModel *Model, VIRTUAL_FILE & VF, int MainOffset, int Object )
{
	VF.Offset = MainOffset;
	//int Diff2 = 8832 - MainBlockOffsets[ 0 ];
	HMMainBlock MainBlock;
	VF.SaveLoad( &MainBlock, sizeof( MainBlock ) );

	VF.Offset = MainBlock.NextOffset;
	int Diff1 = MainOffset - MainBlock.NextOffset;

	HMBlock2 Block2;
	VF.SaveLoad( &Block2, sizeof( Block2 ) );

	int Diff2 = MainBlock.NextOffset - Block2.NextOffset;
	VF.Offset = Block2.NextOffset;

	HMBlock3 Block3;
	VF.SaveLoad( &Block3, sizeof( Block3 ) );

	int Diff3 = Block2.NextOffset - Block3.NextOffset2;
	VF.Offset = Block3.NextOffset2;

	int *Ints = new int[ Diff3 / 4];
	WORD *Words = (WORD*)Ints;
	VF.SaveLoad( Ints, Diff3 );//repeating 3 component WORD structure

	//int Diff4 = Block3.NextOffset2 - Block3.NextOffset;
	//VF.Offset = Block3.NextOffset;

	//int *Ints2 = new int [ Diff4 / 4 ];	
	//WORD *Words2 = (WORD*)Ints2;
	//VF.SaveLoad( Ints2, Diff4 );
	//int *Ints22 = (int*)(((BYTE*)Ints2) + 2);

	int BytesPerVertex = ( Block3.NextOffset2 - Block3.VertexOffset ) / Block3.NumVertices;

	//28 bytes per vertex
	//POS is 12 bytes, 16 extra
	ProcessObject( VF, MainBlock, Model, Block3, Object );
}
void ExtractModels()
{
	char * InFolder = "E:\\Downloads\\Downloaded Torrents\\GMT-MAX.ORG_Hitman_2016_Steam-Rip\\Hitman\\Runtime\\Extract\\PRIM\\";
	//char * InFolder = "C:\\Hitman_2016_Steam-Rip\\Hitman\\Runtime\\Extract\\PRIM\\";
	//const char *File = "00002f36.dat";// "0000372f.dat";
	const char *File = "00003894.dat";
	//const char *File = "000030ee.dat";

	std::string FilePath = InFolder;
	FilePath += File;

	VIRTUAL_FILE VF;
	VF.LoadInMemory( FilePath.c_str() );

	if ( VF.Buffer.Size <= 0 )
	{
		RELog( "File %s not found !", FilePath.c_str() );
		return;
	}

	int EndBlockOffset = -1;
	VF.SaveLoad( &EndBlockOffset );

	//First int32 is file size - "header"
	if ( VF.Buffer.Size - EndBlockOffset != 48 )
	{
		RELog( "EndBlockOffset is %d FileSize is %d !", EndBlockOffset, VF.Buffer.Size );
		return;
	}

	VF.Offset = EndBlockOffset;
	HMEndBlock EndBlock;
	VF.SaveLoad( &EndBlock, sizeof( EndBlock ) );

	int ChunkOffset = EndBlock.TableOffset;
	VF.Offset = ChunkOffset;

	int *MainBlockOffsets = new int[ EndBlock.NumMainEntries ];
	VF.SaveLoad( MainBlockOffsets, sizeof( int ) * EndBlock.NumMainEntries );

	HMModel Model;
	for ( int u = 0; u < EndBlock.NumMainEntries; u++ )
	{
		ProcessMainBlock( &Model, VF, MainBlockOffsets[ u ], u );
	}

	std::string OutFileName = "Exported_1.xml";
	//OutFileName += File;
	char OutPath[ 255 ];
	strcpy( OutPath, OutFileName.c_str() );
	ChangeExtension( OutPath, "xml" );
	//OutFileName = OutPath;
	//OutFileName += ".xml";
	ExportObjectToREM_XML( OutPath, &Model );
	//ExportOBJ( "Exported_a.obj", &Model );
}

int main( int argc, const char **arg)
{
	if ( argc != 3 )
	{
		printf( "Must give 2 paramaters ! first is input folder (Usually a TEXD Directory), second is output folder\n" );
		printf( "Usage\nHitman_2016.exe input_folder output_folder\n" );
		printf( "Example\nHitman_2016.exe Extract\\TEXD Extract\\TEXD\\Out\n" );
		//return -1;
	}
	//ExtractTextures( arg[ 1 ], arg[ 2 ] );
	ExtractModels();
    return 0;
}
