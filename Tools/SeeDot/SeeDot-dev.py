# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

import argparse
from itertools import product
import json
import numpy as np
import os

from seedot.compiler.converter.converter import Converter
from seedot.compiler.converter.bonsai import Bonsai
from seedot.compiler.converter.protonn import Protonn

import seedot.common as Common
from seedot.main import Main
import seedot.util as Util


class Dataset:
    Common = ["cifar-binary", "cr-binary", "cr-multiclass", "curet-multiclass",
              "letter-multiclass", "mnist-binary", "mnist-multiclass",
              "usps-binary", "usps-multiclass", "ward-binary"]
    Extra = ["cifar-10", "eye-binary", "farm-beats",
             "interactive-cane", "whale-binary", "spectakoms", "dsa", "usps10"]
    Default = Common
    All = Common + Extra


class MainDriver:

    def __init__(self):
        self.driversAll = ["compiler", "converter", "predictor"]

    def parseArgs(self):
        parser = argparse.ArgumentParser()

        parser.add_argument("--driver", choices=self.driversAll,
                            metavar='', help="Driver to use")
        parser.add_argument("-a", "--algo", choices=Common.Algo.All,
                            default=Common.Algo.Default, metavar='', help="Algorithm to run")
        parser.add_argument("-v", "--version", choices=Common.Version.All,
                            default=Common.Version.All, metavar='', help="Floating point code or fixed point code")
        parser.add_argument("-d", "--dataset", choices=Dataset.All,
                            default=Dataset.Default, metavar='', help="Dataset to run")
        parser.add_argument("-dt", "--datasetType", choices=Common.DatasetType.All, default=[
                            Common.DatasetType.Default], metavar='', help="Training dataset or testing dataset")
        parser.add_argument("-t", "--target", choices=Common.Target.All, default=[
                            Common.Target.Default], metavar='', help="Desktop code or Arduino code or Fpga HLS code")
        parser.add_argument("-sf", "--max-scale-factor", type=int,
                            metavar='', help="Max scaling factor for code generation")
        parser.add_argument("--load-sf", action="store_true",
                            help="Verify the accuracy of the generated code")
        parser.add_argument("--convert", action="store_true",
                            help="Pass through the converter")
        parser.add_argument("--workers", type=int, default=1, metavar='',
                            help="number of worker threads to parallelize SparseMul on FPGAs only")

        self.args = parser.parse_args()

        if not isinstance(self.args.algo, list):
            self.args.algo = [self.args.algo]
        if not isinstance(self.args.version, list):
            self.args.version = [self.args.version]
        if not isinstance(self.args.dataset, list):
            self.args.dataset = [self.args.dataset]
        if not isinstance(self.args.datasetType, list):
            self.args.datasetType = [self.args.datasetType]
        if not isinstance(self.args.target, list):
            self.args.target = [self.args.target]

    def checkMSBuildPath(self):
        found = False
        for path in Common.msbuildPathOptions:
            if os.path.isfile(path):
                found = True
                Common.msbuildPath = path

        if not found:
            raise Exception("Msbuild.exe not found at the following locations:\n%s\nPlease change the path and run again" % (
                Common.msbuildPathOptions))

    def setGlobalFlags(self):
        np.seterr(all='warn')

    def run(self):
        self.checkMSBuildPath()

        self.setGlobalFlags()

        if self.args.driver is None:
            self.runMainDriver()
        elif self.args.driver == "compiler":
            self.runCompilerDriver()
        elif self.args.driver == "converter":
            self.runConverterDriver()
        elif self.args.driver == "predictor":
            self.runPredictorDriver()

    def runMainDriver(self):

        results = self.loadResultsFile()

        for iter in product(self.args.algo, self.args.version, self.args.dataset, self.args.target):
            algo, version, dataset, target = iter

            print("\n========================================")
            print("Executing on %s %s %s %s" %
                  (algo, version, dataset, target))
            print("========================================\n")

            if self.args.convert:
                datasetDir = os.path.join(
                    "..", "datasets", "datasets", dataset)
                modelDir = os.path.join("..", "model", dataset)

                if algo == Common.Algo.Bonsai:
                    modelDir = os.path.join(
                        modelDir, "BonsaiResults", "Params")
                elif algo == Common.Algo.Lenet:
                    modelDir = os.path.join(modelDir, "LenetModel")
                else:
                    modelDir = os.path.join(modelDir, "ProtoNNResults")

                trainingInput = os.path.join(datasetDir, "training-full.tsv")
                testingInput = os.path.join(datasetDir, "testing.tsv")

                datasetOutputDir = os.path.join(
                    "temp", "dataset-processed", algo, dataset)
                modelOutputDir = os.path.join(
                    "temp", "model-processed", algo, dataset)

                os.makedirs(datasetOutputDir, exist_ok=True)
                os.makedirs(modelOutputDir, exist_ok=True)

                if algo == Common.Algo.Bonsai:
                    obj = Bonsai(trainingInput, testingInput,
                                 modelDir, datasetOutputDir, modelOutputDir)
                    obj.run()
                elif algo == Common.Algo.Protonn:
                    obj = Protonn(trainingInput, testingInput,
                                  modelDir, datasetOutputDir, modelOutputDir)
                    obj.run()

                trainingInput = os.path.join(datasetOutputDir, "train.npy")
                testingInput = os.path.join(datasetOutputDir, "test.npy")
                modelDir = modelOutputDir
            else:
                datasetDir = os.path.join(
                    "..", "dataset-processed", algo, dataset)

                trainingInput = os.path.join(datasetDir, "train.npy")
                testingInput = os.path.join(datasetDir, "test.npy")
                modelDir = os.path.join("..", "model-processed", algo, dataset)

            try:
                if version == Common.Version.Float:
                    key = 'float32'
                elif Common.wordLength == 8:
                    key = 'int8'
                elif Common.wordLength == 16:
                    key = 'int16'
                elif Common.wordLength == 32:
                    key = 'int32'
                else:
                    assert False

                curr = results[algo][key][dataset]

                expectedAcc = curr['accuracy']
                if version == Common.Version.Fixed:
                    bestScale = curr['sf']
                else:
                    bestScale = results[algo]['int16'][dataset]['sf']

            except Exception as _:
                assert self.args.load_sf == False
                expectedAcc = 0

            if self.args.load_sf:
                sf = bestScale
            else:
                sf = self.args.max_scale_factor

            obj = Main(algo, version, target, trainingInput,
                       testingInput, modelDir, sf, self.args.workers)
            obj.run()

            acc = obj.testingAccuracy
            if acc != expectedAcc:
                print("FAIL: Expected accuracy %f%%" % (expectedAcc))
                return
            elif version == Common.Version.Fixed and obj.sf != bestScale:
                print("FAIL: Expected best scale %d" % (bestScale))
                return
            else:
                print("PASS")

    def runCompilerDriver(self):
        for iter in product(self.args.algo, self.args.version, self.args.target):
            algo, version, target = iter

            print("\nGenerating code for " + algo + " " + target + "...")

            inputFile = os.path.join("input", algo + ".sd")
            #inputFile = os.path.join("input", algo + ".pkl")
            profileLogFile = os.path.join("input", "profile.txt")

            outputDir = os.path.join("output")
            os.makedirs(outputDir, exist_ok=True)

            outputFile = os.path.join(outputDir, algo + "-fixed.cpp")
            obj = Main(algo, version, target, inputFile, outputFile,
                       profileLogFile, self.args.max_scale_factor, self.args.workers)
            obj.run()

    def runConverterDriver(self):
        for iter in product(self.args.algo, self.args.version, self.args.dataset, self.args.datasetType, self.args.target):
            algo, version, dataset, datasetType, target = iter

            print("\nGenerating input files for \"" + algo + " " + version +
                  " " + dataset + " " + datasetType + " " + target + "\"...")

            outputDir = os.path.join(
                "Converter", "output", algo + "-" + version + "-" + datasetType, dataset)
            os.makedirs(outputDir, exist_ok=True)

            datasetDir = os.path.join("..", "datasets", "datasets", dataset)
            modelDir = os.path.join("..", "model", dataset)

            if algo == Common.Algo.Bonsai:
                modelDir = os.path.join(modelDir, "BonsaiResults", "Params")
            elif algo == Common.Algo.Lenet:
                modelDir = os.path.join(modelDir, "LenetModel")
            else:
                modelDir = os.path.join(modelDir, "ProtoNNResults")

            trainingInput = os.path.join(datasetDir, "training-full.tsv")
            testingInput = os.path.join(datasetDir, "testing.tsv")

            obj = Converter(algo, version, datasetType, target,
                            outputDir, outputDir, self.args.workers)
            obj.setInput(modelDir, trainingInput, testingInput)
            obj.run()

    def runPredictorDriver(self):
        for iter in product(self.args.algo, self.args.version, self.args.dataset, self.args.datasetType):
            algo, version, dataset, datasetType = iter

            print("\nGenerating input files for \"" + algo + " " +
                  version + " " + dataset + " " + datasetType + "\"...")

            #outputDir = os.path.join("..", "Predictor", algo, version + "-testing")
            #datasetOutputDir = os.path.join("..", "Predictor", algo, version + "-" + datasetType)

            if version == Common.Version.Fixed:
                outputDir = os.path.join(
                    "..", "Predictor", "seedot_fixed", "testing")
                datasetOutputDir = os.path.join(
                    "..", "Predictor", "seedot_fixed", datasetType)
            elif version == Common.Version.Float:
                outputDir = os.path.join(
                    "..", "Predictor", self.algo + "_float", "testing")
                datasetOutputDir = os.path.join(
                    "..", "Predictor", self.algo + "_float", datasetType)

            os.makedirs(datasetOutputDir, exist_ok=True)
            os.makedirs(outputDir, exist_ok=True)

            datasetDir = os.path.join("..", "datasets", "datasets", dataset)
            modelDir = os.path.join("..", "model", dataset)

            if algo == Common.Algo.Bonsai:
                modelDir = os.path.join(modelDir, "BonsaiResults", "Params")
            elif algo == Common.Algo.Lenet:
                modelDir = os.path.join(modelDir, "LenetModel")
            else:
                modelDir = os.path.join(modelDir, "ProtoNNResults")

            trainingInput = os.path.join(datasetDir, "training-full.tsv")
            testingInput = os.path.join(datasetDir, "testing.tsv")

            obj = Converter(algo, version, datasetType, Common.Target.X86,
                            datasetOutputDir, outputDir, self.args.workers)
            obj.setInput(modelDir, trainingInput, testingInput)
            obj.run()

            print("Building and executing " + algo + " " +
                  version + " " + dataset + " " + datasetType + "...")

            outputDir = os.path.join(
                "..", "Predictor", "output", algo + "-" + version)

            curDir = os.getcwd()
            os.chdir(os.path.join("..", "Predictor"))

            obj = Predictor(algo, version, datasetType, outputDir)
            acc = obj.run()

            os.chdir(curDir)

            if acc != None:
                print("Accuracy is %.3f" % (acc))

    def loadResultsFile(self):
        with open(os.path.join("..", "Results", "Results.json")) as data:
            return json.load(data)


if __name__ == "__main__":
    obj = MainDriver()
    obj.parseArgs()
    obj.run()