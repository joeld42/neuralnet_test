// clang -Wno-nullability-completeness ntest.cpp -o ntest -lc++
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <stdlib.h>
#include <string.h>

typedef float real_t;

#define ALLOC(T,num) \
	(T*)malloc(sizeof(T)*(num))

struct Matrix
{
	int rows, cols;
	real_t *data;

	Matrix() : rows(0), cols(0), data(NULL) {}

	Matrix( int _rows, int _cols)
	{		
		_setup( _rows, _cols );
	}

	Matrix( const Matrix &other )
	{
		_setup( other.rows, other.cols );
		_copyDataFrom( other );
	}

	Matrix &operator=( const Matrix &other) {
		if (data) {
			rows = 0; cols = 0;
			free(data);
			data = NULL;
		}
		_setup( other.rows, other.cols );
		_copyDataFrom(other );

		return *this;
	}

	~Matrix()
	{
		free(data);
	}

	void _setup( int _rows, int _cols ) 
	{
		rows = _rows;
		cols = _cols;		
		data = ALLOC(real_t, rows*cols );
	}	

	void _copyDataFrom( const Matrix & other )
	{
		assert( rows*cols == other.rows*other.cols );
		for (int i=0; i < rows*cols; i++) {
			data[i] = other.data[i];
		}
	}

	void Reshape( int _newRows, int _newCols )
	{
		assert( rows*cols == _newCols*_newRows );
		rows = _newRows;
		cols = _newCols;
	}

	void Flip()
	{
		int t = rows;
		rows = cols;
		cols = t;
	}

	real_t get( int i, int j )
	{
		int ndx = i*cols+j;
		return data[ndx];
	}

	void set( int i, int j, real_t v )
	{
		int ndx = i*cols+j;
		data[ndx] = v;
	}

	void accum( int i, int j, real_t v )
	{
		int ndx = i*cols+j;
		data[ndx] += v;
	}

	void print( const char *title )
	{
		char tcopy[32];
		strcpy( tcopy, title );

		for (int j=0; j < rows; j++)
		{
			printf( "%s |", tcopy );
			for (int i=0; i < cols; i++) {
				printf("%5.4f ", get( j, i ));
			}
			printf( "|\n");
			if (j==0) {
				for (char *ch=tcopy; *ch; ch++) {
					*ch = ' ';
				}
			}
		}
		printf("\n");
	}
};

void mat_zero( Matrix &m )
{
	memset( m.data, 0, m.cols * m.rows*sizeof(real_t));
}

void mat_random( Matrix &m )
{
	for (int i=0; i < m.cols*m.rows; i++) {
		m.data[i] = (2.0f * ((real_t)rand() / (real_t)RAND_MAX)) - 1.0f;
	}
}

void mat_scale( Matrix &m, real_t scalar )
{
	for (int i=0; i < m.cols*m.rows; i++) {
		m.data[i] *= scalar;
	}
}


void mat_add( Matrix &dest, Matrix &vals )
{
	assert( dest.rows == vals.rows );
	assert( dest.cols == vals.cols );
	for (int i=0; i < dest.rows*dest.cols; i++) {
		dest.data[i] += vals.data[i];
	}
}

void mat_sub( Matrix &dest, Matrix &vals )
{
	assert( dest.rows == vals.rows );
	assert( dest.cols == vals.cols );
	for (int i=0; i < dest.rows*dest.cols; i++) {
		dest.data[i] -= vals.data[i];
	}
}

void mat_mult( Matrix &dest, Matrix &a, Matrix &b )
{
	// printf("mult %dx%d by %dx%d mat, expect %dx%d result, got %dx%d\n",
	// 		a.rows, a.cols,
	// 		b.rows, b.cols,
	// 		a.rows, b.cols, 
	// 		dest.rows, dest.cols );

	printf("A cols %d, b rows %d\n", a.cols, b.rows );
	assert( b.rows==a.cols );

	// printf("Expect dest to be %dx%d\n", b.cols, a.rows );
	assert( dest.rows==b.cols );
	assert( dest.cols==a.rows );

	mat_zero( dest );

	for (int di=0; di < a.rows; di++)
	{
		for (int dj=0; dj < b.cols; dj++)
		{
			// row a.i dot col b.j
			for (int n=0; n < a.cols; n++) {
				dest.accum( dj, di,  a.get( dj, n ) * b.get( n, di ) );
			}
		}
	}
}

// ---------------------------

struct NetLayer
{
	char layerName[10];
	int num_inputs;
	int num_outputs;
	real_t *inputs;
	real_t *outputs;
	Matrix weights;
	real_t *biases;

	NetLayer( int _numInputs, int _numOutputs, const char *_name=NULL ) :
	weights( _numInputs, _numOutputs )
	{
		num_inputs = _numInputs;
		num_outputs = _numOutputs;
		if (_name) {
			strcpy( layerName, _name );
		} else {
			strcpy( layerName, "Unnamed" );			
		}

		inputs = ALLOC(real_t, num_inputs );
		outputs = ALLOC(real_t, num_outputs );
		biases = ALLOC( real_t, num_outputs );
	}	

	void Describe( )
	{
		printf( "Layer %s\n", layerName );
		printf("Inputs (%d): ", num_inputs );
		for (int i=0; i < num_inputs; i++) {
			printf( "%3.3f ", inputs[i]);
		}
		printf("\n");

		weights.print("Weights");

		printf("Biases (%d): ", num_outputs );
		for (int i=0; i < num_outputs; i++) {
			printf( "%3.3f ", biases[i]);
		}
		printf("\n");

		printf("Outputs (%d): ", num_outputs );
		for (int i=0; i < num_outputs; i++) {
			printf( "%3.3f ", outputs[i]);
		}
		printf("\n");

	}

	void Evaluate()
	{
		for (int i=0; i < num_outputs; i++) {
			//real_t sum = biases[i];
			real_t sum = 0.0;
			for (int j=0; j < num_inputs; j++) {
				sum += inputs[j] * weights.get( i, j );				
			}			
			outputs[i] = sum;
			
			printf("Eval: sum %d %3.2f\n", i, outputs[i] );
		}
		ApplyActivation();
	}

	void ApplyActivation()
	{
		for (int i=0; i < num_outputs; i++) {
			outputs[i] = 1.0 / ( 1.0 + exp(outputs[i]) );
		}
	}
};

// ---------------------------
#if 0
#define MAX_LAYERS (10)
struct NeuralNet
{
	void AddLayer( int layerSize, const char *name )
	{
		lsz[numLayers++] = layerSize;		
		if (numLayers > 1) {
			int n = numLayers - 2;
			int a = lsz[numLayers-2];
			int b = lsz[numLayers-1];
			matWeights[n] = Matrix( b, a );
			mat_random( matWeights[n] );

			matBias[n] = Matrix( b, 1 );
			mat_random( matBias[n] );

			strcpy( layerName[n], name );
		}
	}

	void SetWeight( int layer, int row, int col, float weight )
	{
		assert( layer < numLayers);
		assert( row < lsz[layer] );
		matWeights[layer].set( row, col, weight );
	}

	void SetBias( int layer, int row, float bias )
	{
		assert( layer < numLayers );
		assert( row < lsz[layer] );
		matBias[layer].set( row, 0, bias );
	}

	Matrix Predict( Matrix input ) {

		Matrix dest;

		assert( input.rows == lsz[0] );
		assert( input.cols == 1 );

		for (int i=0; i < numLayers-1; i++)
		{

			// This is stupid because I can't keep track of what the hell is a column
			// vector and what is a row vector
			if (input.cols != matWeights[i].rows) {
				input.Flip();
			}
			printf("Input is %d x %d\n", input.rows, input.cols );
			printf("Layer (%s) is %d x %d\n", layerName[i], matWeights[i].rows, matWeights[i].cols );
			
			dest = Matrix( matWeights[i].cols, 1 );
			mat_mult( dest, input, matWeights[i] );
			
			printf("Layer %d before bias:\n", i);
			dest.print("Layer");

			// dest.print("dest (flip)");
			// matBias[i].print("bias");
			mat_sub( dest, matBias[i] );
			printf("Layer %d (%s) after bias:\n", i, layerName[i]);
			dest.print("Layer");
			
			// apply the activation func
			printf("Layer %d before activation:\n", i);
			dest.print("Layer");
			Activation( dest );
			printf("Layer %d after activation:\n", i);
			dest.print("Layer");

			// printf("Size after layer %d --   %dx%d\n", i, dest.rows, dest.cols );
			
			input = dest;
		}

		// dest.print( "Predict" );		
		return dest;
	}

	void Activation( Matrix &m) 
	{
		for (int i=0; i < m.rows*m.cols; i++) {
			m.data[i] = 1.0 / ( 1.0 + exp(-m.data[i]) );
		}
	}

	void Randomize()
	{
		for (int i=0; i < numLayers; i++) {	
			mat_random( matWeights[i] );

			float d = 1.0 / sqrt( (float)matWeights[i].cols );
			mat_scale( matWeights[i], d );

			mat_random( matBias[i] );
			//mat_zero( matBias[i]);
		}
	}

	void Zero()
	{
		for (int i=0; i < numLayers; i++) {	
			mat_zero( matWeights[i] );

			//mat_random( matBias[i] );
			mat_zero( matBias[i]);
		}
	}

	void Describe()
	{
		int totalWeights = 0;
		for (int i=0; i < numLayers; i++) {
			printf("Layer %d (size %d)\n", i, lsz[i] );
			if ( i > 0) {
				int i2 = i-1;
				printf("   Weights %dx%d biases %d\n", matWeights[i2].rows, matWeights[i2].cols, matBias[i2].rows );
				char buff[20];

				int layerWeights = matWeights[i2].rows * matWeights[i2].cols;
				sprintf( buff, "Weights %d", i2);
				matWeights[i2].print( buff );

				sprintf( buff, "Bias %d", i2 );
				matBias[i2].print( buff );

				totalWeights += layerWeights;
			}

		}		
		printf("Total Weights %d\n", totalWeights );
	}
};
#endif

// ------
void test_net_layer()
{
	NetLayer layer(2, 3, "Layer0");

	layer.inputs[0] = 0.0;
	layer.inputs[1] = 1.0;

	layer.weights.set( 0, 0, -6.029f );
	layer.weights.set( 0, 1, -3.261f );
	layer.weights.set( 1, 0, -5.734f );
	layer.weights.set( 1, 1, -3.172f );
	layer.weights.set( 2, 0, 10.f );
	layer.weights.set( 2, 1, 20.f );

	layer.biases[0] = -1.777;
	layer.biases[1] = -4.460;
	
	layer.Evaluate();
	layer.Describe();

	printf("---------\n");

	NetLayer outlayer( 2, 1, "OutLayer" );

	outlayer.inputs[0] = layer.outputs[0];
	outlayer.inputs[1] = layer.outputs[1];

	outlayer.weights.set( 0, 0, -6.581f );
	outlayer.weights.set( 0, 1, 5.826f );

	outlayer.biases[0] = 2.444f;

	outlayer.Evaluate();
	outlayer.Describe();

	printf("Output >> %1.0f (%3.2f)\n", 
		outlayer.outputs[0],
		outlayer.outputs[1] );
}

#if 0
// ------
void test_basic_mat()
{
	Matrix mA(2, 3);
	mA.set( 0, 0,  1);
	mA.set( 0, 1,  2);
	mA.set( 0, 2,  3);

	mA.set( 1, 0,  4);
	mA.set( 1, 1,  5);
	mA.set( 1, 2,  6);

	mA.print( "Matrix A");	

	Matrix mB(3, 2);

	mB.set( 0, 0,  7);
	mB.set( 0, 1,  8);

	mB.set( 1, 0,  9);
	mB.set( 1, 1,  10);

	mB.set( 2, 0,  11);
	mB.set( 2, 1,  12);

	mB.print( "Matrix B");

	Matrix mResult1( 2, 2 );
	mat_mult( mResult1, mA, mB );
	mResult1.print("A*B");

	Matrix mResult2( 3, 3 );
	mat_mult( mResult2, mB, mA );
	mResult2.print("B*A");
}

void test_basic_net()
{
	NeuralNet net;

	// MNIST setup
	// net.AddLayer( 784 ); // Input layer
	// net.AddLayer( 15 ); // Hidden Layer
	// net.AddLayer( 10 ); // Output Layer
	
	net.AddLayer( 2, "INPUT" );
	net.AddLayer( 3, "HIDDEN" );
	net.AddLayer( 2, "OUTPUT" );

	net.Describe();

	Matrix mInp;
	//mInp.Setup( 784, 1 );
	mat_random( mInp );
	net.Predict( mInp );
}

void test_xor_random_net()
{
	printf("----- xor random test ------\n");
	NeuralNet net;
	NeuralNet bestNet;	
	float bestErr = 0;

	net.AddLayer( 2, "INPUT" );
	net.AddLayer( 3, "HIDDEN" );
	net.AddLayer( 1, "OUTPUT" );

	//net.Describe();

	Matrix mInp( 2, 1 );

 	float input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    float output[4] = {0, 1, 1, 0};

    for (int j=0; j < 20; j++)
    {
    	net.Randomize();
    	float err = 0.0f;
    	//net.Describe();

	    for (int i=0; i < 4; i++)
	    {    	
			mInp.set( 0, 0, input[i][0] );
			mInp.set( 1, 0, input[i][1] );
			
			Matrix result = net.Predict( mInp );			

			float n = result.get(0,0);
			float e = output[i] - n;
			err += (e*e);

			printf("TEST %.f %1.f --> %3.2f (exp: %1.f, %3.2f)\n", 
				input[i][0], input[i][1], 
				n, output[i], 
				fabs( output[i] - n)  );

		}
		if ((err < bestErr) || (j==0)) {
				bestNet = net;		
				bestErr = err;		
			}

		printf("iter %d) err: %f, best %f\n", 
		j, err,bestErr  );		
	}

	for (int i=0; i < 4; i++)
	{    	
		mInp.set( 0, 0, input[i][0] );
		mInp.set( 1, 0, input[i][1] );
		
		Matrix result = net.Predict( mInp );

		float n = result.get(0,0);
		printf("%.f %1.f --> %1.f, %3.2f (exp: %1f)\n", 
			input[i][0], input[i][1], round(n), n, output[i] );

	}
}

void test_xor_saved_net()
{
	printf("----- xor saved net test ------\n");
	NeuralNet net;

	// Test data borrowed from genann library
	// 2 1 2 1 
	// B -1.777 W1 -5.734 -6.029 
	// B -4.460 W2 -3.261 -3.172 
	//
	// 2.444 
	// -6.581 5.826
	net.AddLayer( 2, "INPUTS" ); // Inputs
	net.AddLayer( 2, "HIDDEN" ); // Hidden layer
	net.AddLayer( 1, "OUTPUTS" ); // Outputs
	
	net.SetBias( 0, 0, -1.777 );
	net.SetBias( 0, 1, -4.460 );

	net.SetWeight( 0,   0, 0, -6.029 );
	net.SetWeight( 0,   0, 1, -3.261 );
	net.SetWeight( 0,   1, 0, -5.734 );
	net.SetWeight( 0,   1, 1, -3.172 );

	net.SetBias( 1, 0, 2.444 );
	net.SetWeight( 1,  0, 0, -6.581 );
	net.SetWeight( 1,  0, 1, 5.826 );

	net.Describe();

	Matrix mInp( 2, 1 );

 	float input[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    float output[4] = {0, 1, 1, 0};
    float match_output[4] = { 0.090012, 0.883921, 0.870898, 0.150186 }; // output from genann

	for (int i=0; i < 4; i++)
	{    	
		printf( "---------[ %d ] ------------------------\n", i );
		mInp.set( 0, 0, input[i][0] );
		mInp.set( 1, 0, input[i][1] );
		
		Matrix result = net.Predict( mInp );

		float n = result.get(0,0);
		printf("%.f %1.f --> %1.f, %3.5f (exp: %1f, match %f)\n", 
			input[i][0], input[i][1], round(n), n, output[i], match_output[i] );

	}
}
#endif

int main(int argc, char *argv[] )
{
	//test_basic_mat();
	//test_basic_net();
	//test_xor_random_net();
	//test_xor_saved_net();
	test_net_layer();
}




