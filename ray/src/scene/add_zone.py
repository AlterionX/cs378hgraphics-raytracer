import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.collections import PatchCollection
from matplotlib.patches import Polygon
from matplotlib.backends.backend_pdf import PdfPages
import datetime

import math, os, sys

DATASET_PATH = 'dataset'
def load_dataset(name):
    return pd.read_csv(os.path.join(DATASET_PATH, name))


df_yt = load_dataset('yellow_trips.csv')
print df_yt
