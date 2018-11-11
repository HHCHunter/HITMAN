
struct HeaderVAP
{
	char Unknown[ 16 ];
	int16_t Width;
	int16_t Height;
	BYTE Format;
	char Unknown2[ 71 ];
};

int GetMipSize( TEXTURE_INFO Info );

struct DX10Header
{
	void SetFormat( TEXTURE::FORMAT Format )
	{
		switch ( Format )
		{
			case TEXTURE::BC6U: Type = 0x5F; break;
			case TEXTURE::BC6S: Type = 0x60; break;
			case TEXTURE::BC7: Type = 0x62; break;
		}
	}
	//BYTE Type = 0x5F;//BC6 - U16
	BYTE Type = 0x60;//BC6 - SF16
					 //BYTE Type = 0x62;// BC7
	char Unknown1[ 3 ] = { 0 };
	char Unknown2 = 3;
	char Unknown3[ 7 ] = { 0 };
	char Unknown4 = 1;
	char Unknown5[ 7 ] = { 0 };
};


struct WORD2
{
	WORD X, Y;
	void Length()
	{
		//X = BigEndian16( X );
		//Y = BigEndian16( Y );
		//Z = BigEndian16( Z );
		float FX = (float)X / 65535.0f;
		float FY = (float)Y / 65535.0f;
	}
};

struct WORD3
{
	WORD X, Y, Z;
	void Length()
	{
		X = BigEndian16( X );
		Y = BigEndian16( Y );
		Z = BigEndian16( Z );
		float FX = ( X - 32767 ) / ( float )32767.0f;
		float FY = ( Y - 32767 ) / ( float )32767.0f;
		float FZ = ( Z - 32767 ) / ( float )32767.0f;
		float Norm = sqrt( FX * FX + FY * FY + FZ * FZ );
		Norm = Norm;
	}
};

#pragma pack(1)
struct HMEndBlock
{
	int Ints[ 3 ];
	int NumMainEntries;
	int TableOffset;
	int Ints2[ 7 ];
};
struct HMMainBlock
{
	int Int1;
	WORD W1;
	WORD W2;
	int SomeKindOfID;//multiple of 0x10000 most times
	int Ints[ 2 ];
	Vector3 Bounds[ 2 ];
	int NextOffset;
	Vector4 Transform[ 4 ];
	int Ints2[ 10 ];
	//int Ints2[ 26 ];
};
struct HMBlock2
{
	int NextOffset;
	int Ints[ 3 ];
};
struct HMBlock3
{
	int Unknown;
	int NextOffset;//8192 all the time ?
	int Ints2[ 3 ];
	Vector3 Bounds[ 2 ];
	int NumVertices;
	int VertexOffset;
	int NumIndices;
	int Unknown2;
	int IndicesOffset;
	int NextOffset2;
	int Unknown3;
};

struct HMObject
{
	std::string Name;

	DYNAMIC_ARRAY<uint16_t> Indices;
	DYNAMIC_ARRAY<Vector3> Vertices;

	HMObject():
		Indices( 0, 1000 ),
		Vertices( 0, 1000 )
	{

	}
};
struct HMModel
{
	DYNAMIC_ARRAY< HMObject* > Objects;
};
