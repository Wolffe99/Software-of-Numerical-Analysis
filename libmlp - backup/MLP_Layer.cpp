///////////////////////////////////////////////////////////
//           DO NOT MODIFY THIS FILE                     //
///////////////////////////////////////////////////////////

#include "MLP_Layer.h"

#ifdef _OPENMP
    #include <omp.h>
#endif


void MLP_Layer::Allocate(int previous_num, int current_num)
{
    this->nPreviousNeurons   =  previous_num;
    this->nCurrentNeurons    =  current_num;

    W        = new FLOAT_TYPE[nPreviousNeurons * nCurrentNeurons];
    b        = new FLOAT_TYPE[nCurrentNeurons];

    dW       = new INTERNAL_FLOAT_TYPE[nPreviousNeurons * nCurrentNeurons];
    db       = new INTERNAL_FLOAT_TYPE[nCurrentNeurons];

    outputLayer    = new FLOAT_TYPE[nCurrentNeurons];
    Delta          = new INTERNAL_FLOAT_TYPE[nCurrentNeurons];
    
    FLOAT_TYPE COEF_RANDOM = 2*sqrt(2.0) * sqrt( 6.0 / ( previous_num + current_num) );

    srand((unsigned)time(NULL));
    for (int j = 0; j < nCurrentNeurons; j++)
		{
        outputLayer[j]=0.0;
        Delta[j]=0.0;
        for (int i = 0; i < nPreviousNeurons; i++)
			{
            W[j*nPreviousNeurons+i]  = COEF_RANDOM * rand() / RAND_MAX - COEF_RANDOM/2;
            dW[j*nPreviousNeurons+i] = 0.0;
			}
        b[j]   = COEF_RANDOM * rand() / RAND_MAX - COEF_RANDOM/2;
        db[j]  = 0.0;
		}
}



void MLP_Layer::Delete()
{
    if( W           != nullptr) { delete [] W;           W = nullptr; }
    if( dW          != nullptr) { delete [] dW;          dW = nullptr; }
    if( Delta       != nullptr) { delete [] Delta;       Delta = nullptr; }
    if( outputLayer != nullptr) { delete [] outputLayer; outputLayer = nullptr; }
    if( db          != nullptr) { delete [] db;          db = nullptr; }
    if( b           != nullptr) { delete [] b;           b = nullptr; }
}


FLOAT_TYPE* MLP_Layer::ForwardPropagate(FLOAT_TYPE* inputLayers)      // outputLayer = f( sigma(weights * inputs) + bias )
{
    this->inputLayer=inputLayers;
    for(int j = 0 ; j < nCurrentNeurons ; j++)
    {
        INTERNAL_FLOAT_TYPE net = 0;
#if defined(_OPENMP)
    #pragma omp parallel for simd reduction(+:net) // schedule(simd:static, 5)
#endif
        for(int i = 0 ; i < nPreviousNeurons ; i++)
        {
            net += inputLayer[i] * W[j*nPreviousNeurons+i];
        }
        net+=b[j];
        
        outputLayer[j] = ActivationFunction(net);
    }
    return outputLayer;
}

void MLP_Layer::BackwardPropagateOutputLayer(FLOAT_TYPE *desiredValues)
{
#if defined(_OPENMP)
    #pragma omp parallel for
#endif
    for (int k = 0; k < nCurrentNeurons; k++)
        {
        Delta[k] = DerivativeActivation(outputLayer[k]) * (desiredValues[k] - outputLayer[k]);
        }
    
#if defined(_OPENMP)
    #pragma omp parallel for
#endif
    for (int k = 0 ; k < nCurrentNeurons ; k++)
        for (int j = 0 ; j < nPreviousNeurons; j++)
            dW[k*nPreviousNeurons + j] += - (Delta[k] * inputLayer[j]);

#if defined(_OPENMP)
    #pragma omp parallel for
#endif
    for (int k = 0 ; k < nCurrentNeurons   ; k++)
            db[k] += - Delta[k] ;
}

void MLP_Layer::BackwardPropagateHiddenLayer(MLP_Layer* previousLayer)
{
    FLOAT_TYPE* previousLayer_weight = previousLayer->GetWeight();   // Remind that 'previous' for back progagation means 'next layer' in term of k
    INTERNAL_FLOAT_TYPE* previousLayer_Delta = previousLayer->GetDelta();
    int previousLayer_node_num = previousLayer->GetNumCurrent();

    for (int j = 0; j < nCurrentNeurons; j++)
        {
        INTERNAL_FLOAT_TYPE previous_sum=0;
#if defined(_OPENMP)
    #pragma omp parallel for simd reduction(+:previous_sum) // schedule(simd:static, 5)
#endif
        for (int k = 0; k < previousLayer_node_num; k++)
            {
            previous_sum += previousLayer_Delta[k] * previousLayer_weight[k*nCurrentNeurons + j];
            }
        Delta[j] =  DerivativeActivation(outputLayer[j])* previous_sum;
        }
    
#if defined(_OPENMP)
    #pragma omp parallel for
#endif
    for (int j = 0; j < nCurrentNeurons; j++)
        for (int i = 0; i < nPreviousNeurons ; i++)
            dW[j*nPreviousNeurons + i] +=  -Delta[j] * inputLayer[i];

#if defined(_OPENMP)
    #pragma omp parallel for
#endif
    for (int j = 0 ; j < nCurrentNeurons   ; j++)
        db[j] += -Delta[j] ;
}

int MLP_Layer::GetMaxOutputIndex() const
{
    int maxIdx = 0;

    for(int o = 1; o < nCurrentNeurons; o++)
        {
        if(outputLayer[o] > outputLayer[maxIdx])
            maxIdx = o;
        }
    return maxIdx;
}

float MLP_Layer::GetBinaryOutput() const
{
if( nCurrentNeurons == 1 )   // binaire check
    {
    if(outputLayer[0] > 0.5 )
        return 1.0;
    else
        return 0.0;
    }
return 0.f;
}

ifstream& operator>>(ifstream& is, MLP_Layer& mlp_l)
{
    int nPreviousUnit ;
    int nCurrentUnit  ;

    is.read( reinterpret_cast<char *>(&nPreviousUnit), sizeof(int) );
    is.read( reinterpret_cast<char *>(&nCurrentUnit), sizeof(int) );
    is.read( reinterpret_cast<char *>(&mlp_l.Activation_function), sizeof(char) );

    mlp_l.Allocate(nPreviousUnit, nCurrentUnit);

    is.read(reinterpret_cast<char *>(mlp_l.W)  , sizeof(FLOAT_TYPE) * nPreviousUnit * nCurrentUnit );
    is.read(reinterpret_cast<char *>(mlp_l.b)  , sizeof(FLOAT_TYPE) * nCurrentUnit );

    return is;
}

ofstream& operator<<(ofstream& os, const MLP_Layer& mlp_l)
{
    int nPreviousUnit = mlp_l.nPreviousNeurons;
    int nCurrentUnit  = mlp_l.nCurrentNeurons;

    os.write(reinterpret_cast<const char *>( &(nPreviousUnit) ), sizeof(int) );
    os.write(reinterpret_cast<const char *>( &(nCurrentUnit) ), sizeof(int) );
    os.write(reinterpret_cast<const char *>(&mlp_l.Activation_function), sizeof(char) );

    os.write(reinterpret_cast<const char *>(mlp_l.W)  , sizeof(FLOAT_TYPE) * nPreviousUnit * nCurrentUnit );
    os.write(reinterpret_cast<const char *>(mlp_l.b)  , sizeof(FLOAT_TYPE) * nCurrentUnit );

    return os;
}


