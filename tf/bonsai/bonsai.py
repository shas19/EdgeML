import tensorflow as tf
import numpy as np


class Bonsai:
    def __init__(self, numClasses, dataDimension, projectionDimension,
                 treeDepth, sigma,
                 W=None, T=None, V=None, Z=None):
        '''
        Expected Dimensions:

        Bonsai Params // Optional
        W [numClasses*totalNodes, projectionDimension]
        V [numClasses*totalNodes, projectionDimension]
        Z [projectionDimension, dataDimension + 1]
        T [internalNodes, projectionDimension]

        internalNodes = 2**treeDepth - 1
        totalNodes = 2*internalNodes + 1

        sigma - tanh nonlinearity
        sigmaI - Indicator function for node probs
        sigmaI - has to be set to infinity(1e9 for practicality) while doing testing/inference
        numClasses will be reset to 1 in binary case
        '''

        self.dataDimension = dataDimension
        self.projectionDimension = projectionDimension

        if numClasses == 2:
            self.numClasses = 1
        else:
            self.numClasses = numClasses

        self.treeDepth = treeDepth
        self.sigma = sigma

        self.internalNodes = 2**self.treeDepth - 1
        self.totalNodes = 2 * self.internalNodes + 1

        self.W = self.initW(W)
        self.V = self.initV(V)
        self.T = self.initT(T)
        self.Z = self.initZ(Z)

        self.sigmaI = 1.0

        self.assertInit()

        self.score = None
        self.X_ = None
        self.prediction = None

    def initZ(self, Z):
        if Z is None:
            Z = tf.random_normal(
                [self.projectionDimension, self.dataDimension])
        Z = tf.Variable(Z, name='Z', dtype=tf.float32)
        return Z

    def initW(self, W):
        if W is None:
            W = tf.random_normal(
                [self.numClasses * self.totalNodes, self.projectionDimension])
        W = tf.Variable(W, name='W', dtype=tf.float32)
        return W

    def initV(self, V):
        if V is None:
            V = tf.random_normal(
                [self.numClasses * self.totalNodes, self.projectionDimension])
        V = tf.Variable(V, name='V', dtype=tf.float32)
        return V

    def initT(self, T):
        if T is None:
            T = tf.random_normal(
                [self.internalNodes, self.projectionDimension])
        T = tf.Variable(T, name='T', dtype=tf.float32)
        return T

    def __call__(self, X):
        '''
        Function to build the Bonsai Tree graph
        Expected Dimensions

        X is [_, self.dataDimension]
        '''
        errmsg = "Dimension Mismatch, X is [_, self.dataDimension]"
        assert (len(X.shape) == 2 and int(
            X.shape[1]) == self.dataDimension), errmsg
        if self.score is not None:
            return self.score, self.X_

        X_ = tf.divide(tf.matmul(self.Z, X, transpose_b=True),
                       self.projectionDimension)

        W_ = self.W[0:(self.numClasses)]
        V_ = self.V[0:(self.numClasses)]

        self.__nodeProb = []
        self.__nodeProb.append(1)

        score_ = self.__nodeProb[0] * tf.multiply(
            tf.matmul(W_, X_), tf.tanh(self.sigma * tf.matmul(V_, X_)))
        for i in range(1, self.totalNodes):
            W_ = self.W[i * self.numClasses:((i + 1) * self.numClasses)]
            V_ = self.V[i * self.numClasses:((i + 1) * self.numClasses)]

            T_ = tf.reshape(self.T[int(np.ceil(i / 2) - 1)],
                            [-1, self.projectionDimension])
            prob = (1 + ((-1)**(i + 1)) *
                    tf.tanh(tf.multiply(self.sigmaI, tf.matmul(T_, X_))))

            prob = tf.divide(prob, 2)
            prob = self.__nodeProb[int(np.ceil(i / 2) - 1)] * prob
            self.__nodeProb.append(prob)
            score_ += self.__nodeProb[i] * tf.multiply(
                tf.matmul(W_, X_), tf.tanh(self.sigma * tf.matmul(V_, X_)))

        self.score = score_
        self.X_ = X_
        return self.score, self.X_

    def getPrediction(self):
        '''
        Takes in a score tensor and outputs a integer class for each datapoint
        '''

        if self.prediction is not None:
            return self.prediction

        if self.numClasses > 2:
            self.prediction = tf.argmax(tf.transpose(self.score), 1)
        else:
            self.prediction = tf.argmax(
                tf.concat([tf.transpose(self.score),
                           0 * tf.transpose(self.score)], 1), 1)

        return self.prediction

    def saveModel(self, currDir):
        '''
        Saved the model params as separate numpy dumps
        HyperParam dict as a numpy dump
        '''
        paramDir = currDir + '/'
        np.save(paramDir + "W.npy", self.W.eval())
        np.save(paramDir + "V.npy", self.V.eval())
        np.save(paramDir + "T.npy", self.T.eval())
        np.save(paramDir + "Z.npy", self.Z.eval())
        hyperParamDict = {'dataDim': self.dataDimension,
                          'projDim': self.projectionDimension,
                          'numClasses': self.numClasses,
                          'depth': self.treeDepth, 'sigma': self.sigma}
        hyperParamFile = paramDir + 'hyperParam.npy'
        np.save(hyperParamFile, hyperParamDict)

    def loadModel(self, currDir):
        '''
        Load the Saved model and load it to the model using constructor
        Returns two dict one for params and other for hyperParams
        '''
        paramDir = currDir + '/'
        paramDict = {}
        paramDict['W'] = np.load(paramDir + "W.npy")
        paramDict['V'] = np.load(paramDir + "V.npy")
        paramDict['T'] = np.load(paramDir + "T.npy")
        paramDict['Z'] = np.load(paramDir + "Z.npy")
        hyperParamDict = np.load(paramDir + "hyperParam.npy").item()

        return paramDict, hyperParamDict

    def assertInit(self):
        errRank = "All Parameters must has only two dimensions shape = [a, b]"
        assert len(self.W.shape) == len(self.Z.shape), errRank
        assert len(self.W.shape) == len(self.T.shape), errRank
        assert len(self.W.shape) == 2, errRank
        assert self.W.shape == self.V.shape, "W and V should be of same Dimensions"
        errW = "W and V are [numClasses*totalNodes, projectionDimension]"
        assert self.W.shape[0] == self.numClasses * self.totalNodes, errW
        assert self.W.shape[1] == self.projectionDimension, errW
        errZ = "Z is [projectionDimension, dataDimension]"
        assert self.Z.shape[0] == self.projectionDimension, errZ
        assert self.Z.shape[1] == self.dataDimension, errZ
        errT = "T is [internalNodes, projectionDimension]"
        assert self.T.shape[0] == self.internalNodes, errT
        assert self.T.shape[1] == self.projectionDimension, errT
        assert int(self.numClasses) > 0, "numClasses should be > 1"
        assert int(self.dataDimension) > 0, "# of features in data should be > 0"
        assert int(self.projectionDimension) > 0, "Projection should be  > 0 dims"
        assert int(self.treeDepth) >= 0, "treeDepth should be >= 0"
