import torch
torch.manual_seed(123)
from torch.autograd import Variable
import numpy as np
# Set random seed for reproducible results
np.random.seed(42)
from config import GENRES, DATAPATH, MODELPATH
from model import genreNet
from data import Data
from set import Set
from sklearn.model_selection import StratifiedKFold
import pandas as pd
import matplotlib.pyplot as plt
import os
import csv 

#30 secs - 10 x 3 secs        100x10x10   =10000- 7k train, 2k valid, 1k test
# x2

'''
#test set - pooja 
#test - accuracy, you also check accuracy
# test -model 
# test - 

##......##
blues 00024
classical 00063
disco 00004
reggae 00063
hiphop 00006
metal 00003
rock 00021
pop 00026
country 00008
jazz 00060
'''
import os
import numpy as np
import torch
import matplotlib.pyplot as plt

# =========================
# Setup paths
# =========================
result_path = os.getcwd()
spectogram_path = os.path.join(result_path, "spectogram")
spectogram_csv_path = os.path.join(spectogram_path, "spectogram.csv")

# Create directory if not exists
os.makedirs(spectogram_path, exist_ok=True)

# Initialize CSV if not exists
if not os.path.exists(spectogram_csv_path):
    with open(spectogram_csv_path, "w") as f:
        f.write("file_name,sample_idx,label,max_val,min_val\n")


# =========================
# CSV writer
# =========================
def write_to_csv_spectogram(file_name, sample_idx, label, max_val, min_val):
    with open(spectogram_csv_path, "a") as f:
        f.write(f"{file_name},{sample_idx},{label},{max_val},{min_val}\n")


# =========================
# Plotting function
# =========================


def plot_single_melspectrogram(
    spec,
    label=None,
    file_prefix="melspec",
    save_dir="spectrograms",
    cmap="magma",
    class_names=None,
    ref=1.0, amin=1e-10, top_db=80.0,
    verbose=False
):
    """
    Plot and save a single mel-spectrogram in dB scale.

    Args:
        spec: Tensor or ndarray of shape (1, H, W) or (H, W), linear mel-spectrogram.
        label: Optional label (int or str).
        file_prefix: Prefix for saved filename.
        save_dir: Directory to save image.
        cmap: Colormap for visualization.
        class_names: Optional mapping for label -> name.
        ref: Reference value for dB conversion.
        amin: Minimum amplitude for log scaling.
        top_db: Dynamic range in dB.
        verbose: Print save path if True.
    """
    os.makedirs(save_dir, exist_ok=True)

    # Convert to numpy
    if isinstance(spec, torch.Tensor):
        spec = spec.detach().cpu().numpy()

    # Remove channel if present
    if spec.ndim == 3 and spec.shape[0] == 1:
        spec = spec[0]

    # Convert to dB scale
    spec_db = 10.0 * np.log10(np.maximum(amin, spec))
    spec_db -= 10.0 * np.log10(ref)
    spec_db = np.clip(spec_db, spec_db.max() - top_db, spec_db.max())

    # Plot
    plt.figure(figsize=(4, 4))
    plt.imshow(spec_db, cmap=cmap, aspect="auto", origin="lower")
    title = ""
    if label is not None:
        if class_names is not None:
            if isinstance(class_names, dict):
                title = class_names.get(int(label), str(label))
            else:
                idx = int(label)
                title = class_names[idx] if 0 <= idx < len(class_names) else str(label)
        else:
            title = str(label)
    if title:
        plt.title(f"Label: {title}")
    plt.axis("off")

    save_path = os.path.join(save_dir, f"{file_prefix}.png")
    plt.savefig(save_path, dpi=150, bbox_inches="tight")
    plt.close()

    if verbose:
        print(f"[plot_single_melspectrogram] Saved: {save_path}")
    return save_path


def main():
    
    # denom = train_max - train_min
    data = Data(GENRES, DATAPATH)
    data.make_raw_data()
    data.save()
    data = Data(GENRES, DATAPATH)
    data.load()
    # ------------------------------------------------------------------------------------------- #

   
    fold_accuracies = []
    
    for test_fold in range(1, 11):
        if test_fold != 6:
            continue  # skip everything except fold 6
        fold_name = f"fold{test_fold}"
        print(f"--- Fold {test_fold} ---")
        set_ = Set(data, test_fold)
        set_.make_dataset()
        set_.save()
        set_.load()
        x_train, y_train, train_min, train_max = set_.get_train_set()
        np.savez('48khz_training_norm_params.npz', train_min=train_min, train_max=train_max)

        print('x_train_size: ', x_train.shape)
        print('y_train_size: ', y_train.shape)

        denom = train_max - train_min
        denom[denom == 0] = 1e-8

        x_valid, y_valid = set_.get_valid_set()
        x_test, y_test = set_.get_test_set()
    
        

    #exit(0)

    # ------------------------------------------------------------------------------------------- #

        # Normalize all sets using train's min and max
        x_train = (x_train - train_min) / denom
        x_valid = (x_valid - train_min) / denom
        x_test = (x_test - train_min) / denom

        TRAIN_SIZE  = len(x_train)
        VALID_SIZE  = len(x_valid)
        TEST_SIZE   = len(x_test)

        net = genreNet()
        net.cuda()

        criterion   = torch.nn.CrossEntropyLoss()
        optimizer   = torch.optim.RMSprop(net.parameters(), lr=1e-4)

        EPOCH_NUM   = 3000
        BATCH_SIZE  = 16
        
        for epoch in range(EPOCH_NUM):
            inp_train, out_train    = Variable(torch.from_numpy(x_train)).float().cuda(), Variable(torch.from_numpy(y_train)).long().cuda()
            inp_valid, out_valid    = Variable(torch.from_numpy(x_valid)).float().cuda(), Variable(torch.from_numpy(y_valid)).long().cuda()
            # ------------------------------------------------------------------------------------------------- #
            ## TRAIN PHASE # TRAIN PHASE # TRAIN PHASE # TRAIN PHASE # TRAIN PHASE # TRAIN PHASE # TRAIN PHASE  #
            # ------------------------------------------------------------------------------------------------- #
            train_loss = 0
            optimizer.zero_grad()  # <-- OPTIMIZER
            for i in range(0, TRAIN_SIZE, BATCH_SIZE):
                x_train_batch, y_train_batch = inp_train[i:i + BATCH_SIZE], out_train[i:i + BATCH_SIZE]
                #print('y_train_batch',y_train_batch)
                
                '''
                # Plot only first batch of each epoch for sanity check
                if i == 0:
                    # how many images from the batch to save
                    K = min(8, x_train_batch.size(0))  # or any number you like

                    for k in range(K):
                        label_k = y_train_batch[k].item() if torch.is_tensor(y_train_batch[k]) else int(y_train_batch[k])
                        # fold batch index and sample index into the prefix to get unique filenames
                        plot_single_melspectrogram(
                            x_train_batch[k], 
                            label=label_k,
                            file_prefix=f"epoch{epoch}_train_b{i//BATCH_SIZE}_idx{k}",
                            save_dir="spectrograms",
                            verbose=True
                        )
                
                '''
                
                # If the normalization has to be done, do it here
                pred_train_batch    = net(x_train_batch)
                loss_train_batch    = criterion(pred_train_batch, y_train_batch)
                #Modified by Saurav
                #train_loss          += loss_train_batch.data.cpu().numpy()[0]
                train_loss += loss_train_batch.item()

                loss_train_batch.backward()
            optimizer.step()  # <-- OPTIMIZER

            epoch_train_loss    = (train_loss * BATCH_SIZE) / TRAIN_SIZE
            train_sum           = 0
            for i in range(0, TRAIN_SIZE, BATCH_SIZE):
                pred_train      = net(inp_train[i:i + BATCH_SIZE])
                indices_train   = pred_train.max(1)[1]
                # Modified by Saurav
                #train_sum       += (indices_train == out_train[i:i + BATCH_SIZE]).sum().data.cpu().numpy()[0]
                train_sum += (indices_train == out_train[i:i + BATCH_SIZE]).sum().item()
            train_accuracy  = train_sum / float(TRAIN_SIZE)

            # ------------------------------------------------------------------------------------------------- #
            ## VALIDATION PHASE ## VALIDATION PHASE ## VALIDATION PHASE ## VALIDATION PHASE ## VALIDATION PHASE #
            # ------------------------------------------------------------------------------------------------- #
            valid_loss = 0
            for i in range(0, VALID_SIZE, BATCH_SIZE):
                x_valid_batch, y_valid_batch = inp_valid[i:i + BATCH_SIZE], out_valid[i:i + BATCH_SIZE]

                pred_valid_batch    = net(x_valid_batch)
                loss_valid_batch    = criterion(pred_valid_batch, y_valid_batch)
                #valid_loss          += loss_valid_batch.data.cpu().numpy()[0]
                # Modified by Saurav
                valid_loss += loss_valid_batch.item()

            epoch_valid_loss    = (valid_loss * BATCH_SIZE) / VALID_SIZE
            valid_sum           = 0
            for i in range(0, VALID_SIZE, BATCH_SIZE):
                pred_valid      = net(inp_valid[i:i + BATCH_SIZE])
                indices_valid   = pred_valid.max(1)[1]
                #Modified by Saurav
                #valid_sum       += (indices_valid == out_valid[i:i + BATCH_SIZE]).sum().data.cpu().numpy()[0]
                valid_sum += (indices_valid == out_valid[i:i + BATCH_SIZE]).sum().item()

            valid_accuracy  = valid_sum / float(VALID_SIZE)

            print("Epoch: %d\t\tTrain loss : %.2f\t\tValid loss : %.2f\t\tTrain acc : %.2f\t\tValid acc : %.2f" % \
                (epoch + 1, epoch_train_loss, epoch_valid_loss, train_accuracy, valid_accuracy))
            # ------------------------------------------------------------------------------------------------- #

            if (epoch + 1) % 100 == 0:
                save_path = f"{MODELPATH}_epoch_{epoch + 1}.pt"
                torch.save(net.state_dict(), save_path)
                print(f"-> Model saved at epoch {epoch + 1} to {save_path}")

        # ------------------------------------------------------------------------------------------------- #
        ## SAVE GENRENET MODEL
        # ------------------------------------------------------------------------------------------------- #
        torch.save(net.state_dict(), MODELPATH)
        print('-> ptorch model is saved.')
        # ------------------------------------------------------------------------------------------------- #

        # ------------------------------------------------------------------------------------------------- #
        ## EVALUATE TEST ACCURACY
        # ------------------------------------------------------------------------------------------------- #
        inp_test, out_test = Variable(torch.from_numpy(x_test)).float().cuda(), Variable(torch.from_numpy(y_test)).long().cuda()
        test_sum = 0
        for i in range(0, TEST_SIZE, BATCH_SIZE):
            pred_test       = net(inp_test[i:i + BATCH_SIZE])
            indices_test    = pred_test.max(1)[1]
            #Modified by Saurav
            #test_sum        += (indices_test == out_test[i:i + BATCH_SIZE]).sum().data.cpu().numpy()[0]
            test_sum += (indices_test == out_test[i:i + BATCH_SIZE]).sum().item()
        test_accuracy   = test_sum / float(TEST_SIZE)
        print("Test acc: %.2f" % test_accuracy)
        # ------------------------------------------------------------------------------------------------- #
        fold_accuracies.append(test_accuracy)
        print(f"Fold {test_fold} Accuracy: {test_accuracy:.4f}")

    print(f"\nAverage Accuracy over 10 folds: {np.mean(fold_accuracies):.4f}")
    return


def run_fold(train_idx, valid_idx, X, y, train_min, train_max, EPOCH_NUM=300, BATCH_SIZE=16):
    # Normalize using train fold's min/max
    denom = train_max - train_min
    denom[denom == 0] = 1e-8

    x_train, y_train = X[train_idx], y[train_idx]
    x_valid, y_valid = X[valid_idx], y[valid_idx]

    x_train = (x_train - train_min) / denom
    x_valid = (x_valid - train_min) / denom

    TRAIN_SIZE, VALID_SIZE = len(x_train), len(x_valid)

    # Model
    net = genreNet().cuda()
    criterion = torch.nn.CrossEntropyLoss()
    optimizer = torch.optim.RMSprop(net.parameters(), lr=1e-4)

    inp_train = Variable(torch.from_numpy(x_train)).float().cuda()
    out_train = Variable(torch.from_numpy(y_train)).long().cuda()
    inp_valid = Variable(torch.from_numpy(x_valid)).float().cuda()
    out_valid = Variable(torch.from_numpy(y_valid)).long().cuda()

    for epoch in range(EPOCH_NUM):
        net.train()
        optimizer.zero_grad()
        train_loss = 0
        for i in range(0, TRAIN_SIZE, BATCH_SIZE):
            x_batch, y_batch = inp_train[i:i + BATCH_SIZE], out_train[i:i + BATCH_SIZE]
            pred = net(x_batch)
            loss = criterion(pred, y_batch)
            train_loss += loss.item()
            loss.backward()
        optimizer.step()

    # Validation accuracy
    net.eval()
    valid_sum = 0
    with torch.no_grad():
        for i in range(0, VALID_SIZE, BATCH_SIZE):
            pred = net(inp_valid[i:i + BATCH_SIZE])
            indices = pred.max(1)[1]
            valid_sum += (indices == out_valid[i:i + BATCH_SIZE]).sum().item()
    valid_accuracy = valid_sum / float(VALID_SIZE)
    return valid_accuracy


# def main_10fold():
#     # Prepare data
#     data = Data(GENRES, DATAPATH)
#     data.make_raw_data()
#     data.save()
#     data.load()

#     set_ = Set(data)
#     set_.make_dataset()
#     set_.save()
#     set_.load()

#     # Combine all into one
#     X_all, y_all, train_min, train_max = set_.get_all_data()  # <-- You'll need a helper to return all sets combined
#     X_all = np.array(X_all)
#     y_all = np.array(y_all)

    # skf = StratifiedKFold(n_splits=10, shuffle=True, random_state=42)
    # fold_accuracies = []

    # for fold, (train_idx, valid_idx) in enumerate(skf.split(X_all, y_all), 1):
    #     print(f"--- Fold {fold} ---")
    #     acc = run_fold(train_idx, valid_idx, X_all, y_all, train_min, train_max)
    #     fold_accuracies.append(acc)
    #     print(f"Fold {fold} Accuracy: {acc:.4f}")

    # print(f"\nAverage Accuracy over 10 folds: {np.mean(fold_accuracies):.4f}"

    import pandas as pd

if __name__ == '__main__':
    main()
    # main_10fold()





