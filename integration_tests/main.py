from behave.__main__ import main as behave_main
import os
import sys

if __name__ == "__main__":
    featuresPath = os.path.join(os.path.dirname(__file__), "features")
    args = [featuresPath] + sys.argv[1:]
    behave_main(args)