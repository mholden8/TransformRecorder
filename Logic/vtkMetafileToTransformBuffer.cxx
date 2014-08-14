
#include "vtkMetafileToTransformBuffer.h"

#include "vtkObjectFactory.h"

// Helper methods ====================================================================================

//-------------------------------------------------------
void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n")+1);
  str.erase(0,str.find_first_not_of(" \t\r\n"));
}

//----------------------------------------------------------------------------
template<class T>
void StringToInt(const char* strPtr, T &result)
{
  if (strPtr==NULL || strlen(strPtr) == 0 )
  {
    return;
  }
  char * pEnd=NULL;
  result = static_cast<int>(strtol(strPtr, &pEnd, 10));
  if (pEnd != strPtr+strlen(strPtr) )
  {
    return;
  }
  return;
}

// ----------------------------------------------------------------------------
void MatrixFromString( vtkMatrix4x4* matrix, std::string str )
{
  std::stringstream ss( str );

  double e00; ss >> e00; double e01; ss >> e01; double e02; ss >> e02; double e03; ss >> e03;
  double e10; ss >> e10; double e11; ss >> e11; double e12; ss >> e12; double e13; ss >> e13;
  double e20; ss >> e20; double e21; ss >> e21; double e22; ss >> e22; double e23; ss >> e23;
  double e30; ss >> e30; double e31; ss >> e31; double e32; ss >> e32; double e33; ss >> e33;

  matrix->SetElement( 0, 0, e00 );
  matrix->SetElement( 0, 1, e01 );
  matrix->SetElement( 0, 2, e02 );
  matrix->SetElement( 0, 3, e03 );

  matrix->SetElement( 1, 0, e10 );
  matrix->SetElement( 1, 1, e11 );
  matrix->SetElement( 1, 2, e12 );
  matrix->SetElement( 1, 3, e13 );

  matrix->SetElement( 2, 0, e20 );
  matrix->SetElement( 2, 1, e21 );
  matrix->SetElement( 2, 2, e22 );
  matrix->SetElement( 2, 3, e23 );

  matrix->SetElement( 3, 0, e30 );
  matrix->SetElement( 3, 1, e31 );
  matrix->SetElement( 3, 2, e32 );
  matrix->SetElement( 3, 3, e33 );
}


// ----------------------------------------------------------------------------
std::string MatrixToString( vtkMatrix4x4* matrix )
{
  std::stringstream ss;
  
  ss << matrix->GetElement( 0, 0 ) << " ";
  ss << matrix->GetElement( 0, 1 ) << " ";
  ss << matrix->GetElement( 0, 2 ) << " ";
  ss << matrix->GetElement( 0, 3 ) << " ";
  
  ss << matrix->GetElement( 1, 0 ) << " ";
  ss << matrix->GetElement( 1, 1 ) << " ";
  ss << matrix->GetElement( 1, 2 ) << " ";
  ss << matrix->GetElement( 1, 3 ) << " ";
  
  ss << matrix->GetElement( 2, 0 ) << " ";
  ss << matrix->GetElement( 2, 1 ) << " ";
  ss << matrix->GetElement( 2, 2 ) << " ";
  ss << matrix->GetElement( 2, 3 ) << " ";
  
  ss << matrix->GetElement( 3, 0 ) << " ";
  ss << matrix->GetElement( 3, 1 ) << " ";
  ss << matrix->GetElement( 3, 2 ) << " ";
  ss << matrix->GetElement( 3, 3 ) << " ";

  return ss.str();
}


// Constants for reading transforms
static const int MAX_LINE_LENGTH = 1000;

static std::string SEQMETA_FIELD_FRAME_FIELD_PREFIX = "Seq_Frame";
static std::string SEQMETA_FIELD_IMG_STATUS = "ImageStatus";



// Class methods =====================================================================================
vtkStandardNewMacro( vtkMetafileToTransformBuffer );

vtkMetafileToTransformBuffer
::vtkMetafileToTransformBuffer()
{
}


vtkMetafileToTransformBuffer
::~vtkMetafileToTransformBuffer()
{
}


void
vtkMetafileToTransformBuffer
::PrintSelf( ostream& os, vtkIndent indent )
{
  Superclass::PrintSelf( os, indent );
}


// Main method for importing MHA files =============================================================

void vtkMetafileToTransformBuffer
::ImportFromMHAFile( vtkMRMLTransformBufferNode* bufferNode, std::string fileName )
{
  // Open in binary mode because we determine the start of the image buffer also during this read
  const char* flags = "rb";
  FILE* stream = fopen( fileName.c_str(), flags ); // TODO: Removed error

  char line[ MAX_LINE_LENGTH + 1 ] = { 0 };

  // It contains the largest frame number. It will be used to iterate through all the frame numbers from 0 to lastFrameNumber
  int lastFrameNumber = -1;
  std::map< int, std::string > FrameNumberToTimestamp;
  std::map< int, std::vector< vtkTransformRecord* > > FrameNumberToTransformRecords;
  std::map< int, std::map< std::string, bool > > FrameNumberToStatuses;

  while ( fgets( line, MAX_LINE_LENGTH, stream ) )
  {
    std::string lineStr = line;

    // Split line into name and value
    size_t equalSignFound = 0;
    equalSignFound = lineStr.find_first_of( "=" );
    if ( equalSignFound == std::string::npos )
    {
      vtkWarningMacro( "Parsing line failed, equal sign is missing (" << lineStr << ")" );
      continue;
    }
    std::string name = lineStr.substr( 0, equalSignFound );
    std::string value = lineStr.substr( equalSignFound + 1 );

    // Trim spaces from the left and right
    Trim( name );
    Trim( value );

    if ( name.compare( "ElementDataFile" ) == NULL )
    {
      // This is the last field of the header
      break;
    }


    // Only consider the Seq_Frame
    if ( name.compare( 0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size(), SEQMETA_FIELD_FRAME_FIELD_PREFIX ) != 0 )
    {
      // nNt a frame field, ignore it
      continue;
    }

    // frame field
    // name: Seq_Frame0000_CustomTransform
    name.erase( 0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size() ); // 0000_CustomTransform

    // Split line into name and value
    size_t underscoreFound;
    underscoreFound = name.find_first_of( "_" );
    if ( underscoreFound == std::string::npos )
    {
      vtkWarningMacro( "Parsing line failed, underscore is missing from frame field name (" << lineStr << ")" );
      continue;
    }

    std::string frameNumberStr = name.substr( 0, underscoreFound ); // 0000
    std::string frameFieldName = name.substr( underscoreFound + 1 ); // CustomTransform

    int frameNumber = 0;
    StringToInt( frameNumberStr.c_str(), frameNumber ); // TODO: Removed warning
    if ( frameNumber > lastFrameNumber )
    {
      lastFrameNumber = frameNumber;
    }

    // Convert the string to transform and add transform to hierarchy
    if ( frameFieldName.find( "Transform" ) != std::string::npos && frameFieldName.find( "Status" ) == std::string::npos )
    {
      // Find the transform name (i.e. remove the "Transform" from the end)
      size_t transformFound;
      transformFound = frameFieldName.find( "Transform" );
      std::string transformName = frameFieldName.substr( 0, transformFound );

      std::stringstream transformInverseNameStream;
      transformInverseNameStream << transformName << "Inverse";
      std::string transformInverseName = transformInverseNameStream.str();

      // Create the transform record
      vtkTransformRecord* transformRecord =  vtkTransformRecord::New();
      transformRecord->SetTransform( value );
      transformRecord->SetDeviceName( transformName );
      transformRecord->SetTime( frameNumber );

      // Find the inverse of the matrix
      vtkSmartPointer< vtkMatrix4x4 > matrix = vtkSmartPointer< vtkMatrix4x4 >::New();
      MatrixFromString( matrix, value );
      matrix->Invert();
      std::string inverseValue = MatrixToString( matrix );

      // Add the inverse record too
      vtkTransformRecord* transformRecordInverse =  vtkTransformRecord::New();
      transformRecordInverse->SetTransform( inverseValue );
      transformRecordInverse->SetDeviceName( transformInverseName );
      transformRecordInverse->SetTime( frameNumber );

      FrameNumberToTransformRecords[ frameNumber ].push_back( transformRecord );
      FrameNumberToTransformRecords[ frameNumber ].push_back( transformRecordInverse );
    }

    // Find the transform status
    if ( frameFieldName.find( "Transform" ) != std::string::npos && frameFieldName.find( "Status" ) != std::string::npos )
    {
      // Find the transform name (i.e. remove the "Transform" from the end)
      size_t transformFound;
      transformFound = frameFieldName.find( "Transform" );
      std::string transformName = frameFieldName.substr( 0, transformFound );

      std::stringstream transformInverseNameStream;
      transformInverseNameStream << transformName << "Inverse";
      std::string transformInverseName = transformInverseNameStream.str();

      if ( value.compare( "INVALID" ) == 0 )
      {
        FrameNumberToStatuses[ frameNumber ][ transformName ] = false;
        FrameNumberToStatuses[ frameNumber ][ transformInverseName ] = false;
      }
      else
      {
        FrameNumberToStatuses[ frameNumber ][ transformName ] = true;
        FrameNumberToStatuses[ frameNumber ][ transformInverseName ] = true;
      }

    }

    if ( frameFieldName.find( "Timestamp" ) != std::string::npos )
    {
      FrameNumberToTimestamp[ frameNumber ] = value;
    }

    if ( ferror( stream ) )
    {
      vtkErrorMacro( "Error reading the file " << fileName.c_str() );
      break;
    }
    
    if ( feof( stream ) )
    {
      break;
    }

  }
  fclose( stream );

  // Now put all the transform records into the transform buffer  
  for ( int i = 0; i < lastFrameNumber; i++ )
  {
    std::stringstream ss( FrameNumberToTimestamp[ i ] );
    double currentTime; ss >> currentTime;

    for ( int j = 0; j < FrameNumberToTransformRecords[ i ].size(); j++ )
    {
      // Skip if the transform is invalid at the time
      if ( ! FrameNumberToStatuses[ i ][ FrameNumberToTransformRecords[ i ].at( j )->GetDeviceName() ] )
      {
        continue;
      }

      FrameNumberToTransformRecords[ i ].at( j )->SetTime( currentTime );
      bufferNode->AddTransform( FrameNumberToTransformRecords[ i ].at( j ) );
    }    
  }

  FrameNumberToTimestamp.clear();
  FrameNumberToTransformRecords.clear();
  FrameNumberToStatuses.clear();
}