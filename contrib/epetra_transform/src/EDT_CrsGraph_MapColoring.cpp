
#include <EDT_CrsGraph_MapColoring.h>

#include <EDT_CrsGraph_Transpose.h>

#include <vector>

#include <Epetra_CrsGraph.h>
#include <Epetra_MapColoring.h>
#include <Epetra_Map.h>

namespace Epetra_Transform {

std::auto_ptr<Epetra_MapColoring> CrsGraph_MapColoring::operator()( const Epetra_CrsGraph & original )
{
  int err;

  const Epetra_BlockMap & RowMap = original.RowMap();
  int nRows = RowMap.NumMyElements();

  int NumIndices;
  int * Indices;

  //Generate Local Distance-1 Adjacency Graph
  //(Transpose of original excluding non-local column indices)
  std::auto_ptr<Epetra_CrsGraph> Adj1 = CrsGraph_Transpose( true )( original );
  if( verbose_ ) cout << "Adjacency 1 Graph!\n" << *Adj1;

  int Delta = Adj1->MaxNumIndices();
  cout << endl << "Delta: " << Delta << endl;

  //Generation of Local Distance-2 Adjacency Graph
  Epetra_CrsGraph Adj2( Copy, RowMap, 0 );
  int NumAdj1Indices;
  int * Adj1Indices;
  for( int i = 0; i < nRows; ++i )
  {
    assert( Adj1->ExtractMyRowView( i, NumAdj1Indices, Adj1Indices ) == 0 );

    for( int j = 0; j < NumAdj1Indices; ++j )
    {
      assert( original.ExtractMyRowView( Adj1Indices[j], NumIndices, Indices ) == 0 );
      assert( Adj2.InsertMyIndices( i, NumIndices, Indices ) >= 0 );
    }
  }
  assert( Adj2.TransformToLocal() == 0 );
  if( verbose_ ) cout << "Adjacency 2 Graph!\n" << Adj2;

  Epetra_MapColoring * ColorMap = new Epetra_MapColoring( RowMap );

  //Application of Greedy Algorithm to generate Color Map
  int Size = Delta * Delta + 1;
  vector<int> allowedColors( Size );
  for( int col = 0; col < nRows; ++col )
  {
    for( int i = 0; i < Size; ++i ) allowedColors[i] = i+1; 

    Adj2.ExtractMyRowView( col, NumIndices, Indices );

    for( int i = 0; i < NumIndices; ++i )
      if( (*ColorMap)[ Indices[i] ] > 0 ) allowedColors[ (*ColorMap)[ Indices[i] ] - 1 ] = -1;

    for( int i = 1; i <= Size; ++i )
      if( allowedColors[i-1] == i )
      {
        (*ColorMap)[ col ] = i;
        break;
      }
  }

  if( verbose_ ) cout << "ColorMap!\n" << *ColorMap;

  return std::auto_ptr<Epetra_MapColoring> ( ColorMap );
}

}
