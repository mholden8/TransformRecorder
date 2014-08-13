
#ifndef VTKMETAFILETOTRANSFORMBUFFER_H
#define VTKMETAFILETOTRANSFORMBUFFER_H


#include <iostream>
#include <utility>
#include <vector>

#include "vtkSlicerTransformRecorderModuleLogicExport.h"

#include "vtkObject.h"
#include "vtkSmartPointer.h"

#include "vtkMRMLTransformBufferNode.h"


// This class is designed to facilitate importing sequence metafiles into the PerkTutor
class VTK_SLICER_TRANSFORMRECORDER_MODULE_LOGIC_EXPORT
vtkMetafileToTransformBuffer
 : public vtkObject
{

public:
  
  static vtkMetafileToTransformBuffer *New();
  void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro( vtkMetafileToTransformBuffer, vtkObject );

  void ImportFromMHAFile( vtkMRMLTransformBufferNode* bufferNode, std::string fileName ); 

protected:

  vtkMetafileToTransformBuffer();
  ~vtkMetafileToTransformBuffer();

private:
  
  vtkMetafileToTransformBuffer( const vtkMetafileToTransformBuffer& ); // Not implemented.
  void operator=( const vtkMetafileToTransformBuffer& ); // Not implemented.  
};


#endif
