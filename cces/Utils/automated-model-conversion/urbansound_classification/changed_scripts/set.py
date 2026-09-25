import pandas as pd
import numpy as np
np.random.seed(123)
import pickle

from sklearn.preprocessing import LabelEncoder
from sklearn.utils import shuffle

from config import SET_DATAPATH


class Set():

    def __init__(self, data, test_fold):
        self.train_set  = None
        self.valid_set  = None
        self.test_set   = None
        self.data       = data
        self.le         = self.data.GENRES#LabelEncoder().fit(self.data.GENRES)
        self.test_fold = test_fold

        print("\n-> Set() object is initialized.")

    def make_dataset(self):
        df  = self.data.raw_data.copy()
        df  = shuffle(df)

        # Test set: all samples from the selected fold
        test_df = df[df['fold'] == f'fold{self.test_fold}']
        trainval_df = df[df['fold'] != f'fold{self.test_fold}']

        print("trainval_df shape: ", trainval_df.shape)
        print("test_df shape: ", test_df.shape)

        train_records, valid_records, test_records = list(), list(), list()
        for genre in self.data.GENRES:
            genre_df = trainval_df[trainval_df['class_names'] == genre]
            genre_df = shuffle(genre_df)
            n_train = int(0.7 * len(genre_df))
            train_records.append(genre_df.iloc[:n_train].values)
            valid_records.append(genre_df.iloc[n_train:].values)


        train_records = shuffle([rec for genre_recs in train_records for rec in genre_recs])
        valid_records = shuffle([rec for genre_recs in valid_records for rec in genre_recs])

        self.train_set = pd.DataFrame.from_records(train_records, columns=['fold', 'classID', 'class_names', 'spectrogram'])
        self.valid_set = pd.DataFrame.from_records(valid_records, columns=['fold', 'classID', 'class_names', 'spectrogram'])
        self.test_set  = test_df[['fold', 'classID', 'class_names', 'spectrogram']].reset_index(drop=True)
        return


    def get_train_set(self):
        x_train = np.stack(self.train_set['spectrogram'].values)
        x_train = np.reshape(x_train, (x_train.shape[0], 1, x_train.shape[1], x_train.shape[2]))
        y_train = np.stack(self.train_set['classID'].values)
        # y_train = self.le.transform(y_train)

        train_min = x_train.min(axis=(0, 2), keepdims=True)
        train_max = x_train.max(axis=(0, 2), keepdims=True)
        print("x_train inside the code shape: ", x_train.shape) # (7000, 1, 128, 128)
        print("y_train inside the code shape: ", y_train.shape) # (7000)
        '''
        7000 samples (spectrograms)
        1 channel (like an image channel)
        128 time frames
        128 mel-frequency bins
        '''
        return x_train, y_train, train_min, train_max

    def get_valid_set(self):
        x_valid = np.stack(self.valid_set['spectrogram'].values)
        x_valid = np.reshape(x_valid, (x_valid.shape[0], 1, x_valid.shape[1], x_valid.shape[2]))
        y_valid = np.stack(self.valid_set['classID'].values)
        # y_valid = self.le.transform(y_valid)
        print("x_valid shape: ", x_valid.shape)
        print("y_valid shape: ", y_valid.shape)
        return x_valid, y_valid

    def get_test_set(self):
        x_test  = np.stack(self.test_set['spectrogram'].values)
        x_test  = np.reshape(x_test, (x_test.shape[0], 1, x_test.shape[1], x_test.shape[2]))
        y_test  = np.stack(self.test_set['classID'].values)
        # y_test  = self.le.transform(y_test)
        print("x_test shape : ", x_test.shape)
        print("y_test shape : ", y_test.shape)
        return x_test, y_test

    def save(self):
        with open(SET_DATAPATH, 'wb') as outfile:
            pickle.dump((self.train_set, self.valid_set, self.test_set), outfile, pickle.HIGHEST_PROTOCOL)
        print("-> Set() object is saved.\n")
        return

    def load(self):
        with open(SET_DATAPATH, 'rb') as infile:
            (self.train_set, self.valid_set, self.test_set) = pickle.load(infile)
        print("-> Set() object is loaded.\n")
        return
