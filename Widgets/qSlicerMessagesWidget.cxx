/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// FooBar Widgets includes
#include "qSlicerMessagesWidget.h"

#include <QtGui>



//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_CreateModels
class qSlicerMessagesWidgetPrivate
  : public Ui_qSlicerMessagesWidget
{
  Q_DECLARE_PUBLIC(qSlicerMessagesWidget);
protected:
  qSlicerMessagesWidget* const q_ptr;

public:
  qSlicerMessagesWidgetPrivate( qSlicerMessagesWidget& object);
  ~qSlicerMessagesWidgetPrivate();
  virtual void setupUi(qSlicerMessagesWidget*);
};

// --------------------------------------------------------------------------
qSlicerMessagesWidgetPrivate
::qSlicerMessagesWidgetPrivate( qSlicerMessagesWidget& object) : q_ptr(&object)
{
}

qSlicerMessagesWidgetPrivate
::~qSlicerMessagesWidgetPrivate()
{
}


// --------------------------------------------------------------------------
void qSlicerMessagesWidgetPrivate
::setupUi(qSlicerMessagesWidget* widget)
{
  this->Ui_qSlicerMessagesWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerMessagesWidget methods

//-----------------------------------------------------------------------------
qSlicerMessagesWidget
::qSlicerMessagesWidget(QWidget* parentWidget) : qSlicerWidget( parentWidget ) , d_ptr( new qSlicerMessagesWidgetPrivate(*this) )
{
  this->TransformBufferNode = NULL;
  this->TransformRecorderLogic = vtkSlicerTransformRecorderLogic::SafeDownCast( vtkSlicerTransformRecorderLogic::GetSlicerModuleLogic( "TransformRecorder" ) );
  this->setup();
}


qSlicerMessagesWidget
::~qSlicerMessagesWidget()
{
}


void qSlicerMessagesWidget
::setup()
{
  Q_D(qSlicerMessagesWidget);

  d->setupUi(this);

  connect( d->AddMessageButton, SIGNAL( clicked() ), this, SLOT( onAddMessageButtonClicked() ) );
  connect( d->RemoveMessageButton, SIGNAL( clicked() ), this, SLOT( onRemoveMessageButtonClicked() ) ); 
  connect( d->ClearMessagesButton, SIGNAL( clicked() ), this, SLOT( onClearMessagesButtonClicked() ) );

  d->AddMessageButton->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( d->AddMessageButton, SIGNAL( customContextMenuRequested(const QPoint&) ), this, SLOT( onAddBlankMessageClicked() ) );

  connect( d->MessagesTableWidget, SIGNAL( cellChanged( int, int ) ), this, SLOT( onMessageEdited( int, int ) ) );
  connect( d->MessagesTableWidget, SIGNAL( cellDoubleClicked( int, int ) ), this, SLOT( onMessageDoubleClicked( int, int ) ) );

  this->updateWidget();  
}


void qSlicerMessagesWidget
::setTransformBufferNode( vtkMRMLNode* newTransformBufferNode )
{
  Q_D(qSlicerMessagesWidget);

  this->qvtkDisconnectAll();

  this->TransformBufferNode = vtkMRMLTransformBufferNode::SafeDownCast( newTransformBufferNode );

  this->qvtkConnect( this->TransformBufferNode, vtkMRMLTransformBufferNode::MessageAddedEvent, this, SLOT( updateWidget() ) );
  this->qvtkConnect( this->TransformBufferNode, vtkMRMLTransformBufferNode::MessageRemovedEvent, this, SLOT( updateWidget() ) );
  this->qvtkConnect( this->TransformBufferNode, vtkMRMLTransformBufferNode::TransformAddedEvent, this, SLOT( updateWidget() ) );
  this->qvtkConnect( this->TransformBufferNode, vtkMRMLTransformBufferNode::TransformRemovedEvent, this, SLOT( updateWidget() ) );
  // Have to listen to the transform add/remove events in case that changes the relative time of the messages

  this->updateWidget();
}


void qSlicerMessagesWidget
::onAddMessageButtonClicked()
{
  Q_D(qSlicerMessagesWidget);

  if ( this->TransformBufferNode == NULL )
  {
    return;
  }

  double time = this->TransformBufferNode->GetCurrentTimestamp();

  QString messageName = QInputDialog::getText( this, tr("Add Message"), tr("Input text for the new message:") );

  if ( messageName.isNull() )
  {
    return;
  }

  // Record the timestamp
  this->TransformRecorderLogic->AddMessage( this->TransformBufferNode, messageName.toStdString(), time );
  
  this->updateWidget();
}


void qSlicerMessagesWidget
::onRemoveMessageButtonClicked()
{
  Q_D(qSlicerMessagesWidget);

  if ( this->TransformBufferNode == NULL )
  {
    return;
  }

  this->TransformRecorderLogic->RemoveMessage( this->TransformBufferNode, d->MessagesTableWidget->currentRow() );

  this->updateWidget();
}


void qSlicerMessagesWidget
::onClearMessagesButtonClicked()
{
  Q_D(qSlicerMessagesWidget);

  if ( this->TransformBufferNode == NULL )
  {
    return;
  }

  this->TransformRecorderLogic->ClearMessages( this->TransformBufferNode );
  
  this->updateWidget();
}


void qSlicerMessagesWidget
::onAddBlankMessageClicked()
{
  Q_D(qSlicerMessagesWidget);

  if ( this->TransformBufferNode == NULL )
  {
    return;
  }

  double time = this->TransformBufferNode->GetCurrentTimestamp();
  this->TransformRecorderLogic->AddMessage( this->TransformBufferNode, "", time );
  
  this->updateWidget();
}



void qSlicerMessagesWidget
::onMessageEdited( int row, int column )
{
  Q_D(qSlicerMessagesWidget);

  if ( this->TransformBufferNode == NULL || column == qSlicerMessagesWidget::MESSAGE_TIME_COLUMN )
  {
    return;
  }

  // Find the entry that we changed
  QTableWidgetItem* qItem = d->MessagesTableWidget->item( row, column );
  QString qText = qItem->text();

  this->TransformBufferNode->GetMessageAtIndex( row )->SetMessageString( qText.toStdString() );
  
  this->updateWidget();
}


void qSlicerMessagesWidget
::onMessageDoubleClicked( int row, int column )
{
  // Do nothing, with the understanding that sublasses can implement this slot
}


void qSlicerMessagesWidget
::updateWidget()
{
  Q_D(qSlicerMessagesWidget);

  // Check what the current row and column are
  int currentRow = d->MessagesTableWidget->currentRow();
  int currentColumn = d->MessagesTableWidget->currentColumn();
  int scrollPosition = d->MessagesTableWidget->verticalScrollBar()->value();
  
  // The only thing to do is update the table entries. Must ensure they are in sorted order (that's how they are stored in the buffer).
  d->MessagesTableWidget->clear();
  QStringList MessagesTableHeaders;
  MessagesTableHeaders << "Time" << "Message";
  d->MessagesTableWidget->setRowCount( 0 );
  d->MessagesTableWidget->setColumnCount( 2 );
  d->MessagesTableWidget->setHorizontalHeaderLabels( MessagesTableHeaders ); 
  d->MessagesTableWidget->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  if ( this->TransformBufferNode == NULL )
  {
    return;
  }

  // Block signals while updating
  bool wasBlockedTableWidget = d->MessagesTableWidget->blockSignals( true );

  // Iterate over all the messages in the buffer and add them in order
  d->MessagesTableWidget->setRowCount( this->TransformBufferNode->GetNumMessages() );
  for ( int i = 0; i < this->TransformBufferNode->GetNumMessages(); i++ )
  {
    double messageTime = this->TransformBufferNode->GetMessageAtIndex(i)->GetTime() - this->TransformBufferNode->GetMinimumTime();
    QTableWidgetItem* timeItem = new QTableWidgetItem( QString::number( messageTime, 'f', 2 ) );
    timeItem->setFlags( timeItem->flags() & ~Qt::ItemIsEditable );
	  QTableWidgetItem* messageItem = new QTableWidgetItem( QString::fromStdString( this->TransformBufferNode->GetMessageAtIndex(i)->GetMessageString() ) );
    d->MessagesTableWidget->setItem( i, qSlicerMessagesWidget::MESSAGE_TIME_COLUMN, timeItem );
    d->MessagesTableWidget->setItem( i, qSlicerMessagesWidget::MESSAGE_NAME_COLUMN, messageItem ); 
  }

  // Reset the current row and column to what they were
  d->MessagesTableWidget->setCurrentCell( currentRow, currentColumn );
  d->MessagesTableWidget->verticalScrollBar()->setValue( scrollPosition );

  d->MessagesTableWidget->blockSignals( wasBlockedTableWidget );
}
