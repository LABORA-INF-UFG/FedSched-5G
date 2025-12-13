import pandas as pd
from ml_model.ml_model import Model
import numpy as np
import tensorflow as tf


class Client:
    def __init__(self, cid, scheduler, model_type, path_clients):
        self.cid = int(cid)
        self.scheduler = scheduler
        self.model_type = model_type
        self.path_clients = path_clients

        if self.model_type == "CNN":
            self.model = Model.create_model_cnn()
        else:
            self.model = Model.create_model_mlp()

        (self.x_train, self.y_train), (self.x_test, self.y_test) = (None, None), (None, None)

    def load_data(self):
        train = pd.read_pickle(f"{self.path_clients}/{self.cid}_train.pickle")
        test = pd.read_pickle(f"{self.path_clients}/{self.cid}_test.pickle")


        x_train = train.drop(['label'], axis=1)
        y_train = train['label']

        x_test = test.drop(['label'], axis=1)
        y_test = test['label']

        x_train /= 255.0
        x_test /= 255.0

        if self.model_type == "CNN":
            x_train = np.array(
                [x.reshape(28, 28) for x in x_train.reset_index(drop=True).values])
            x_test = np.array(
                [x.reshape(28, 28) for x in x_test.reset_index(drop=True).values])

        return (x_train, y_train), (x_test, y_test)

    def fit_defaut(self, parameters, config=None):
        (self.x_train, self.y_train), (self.x_test, self.y_test) = self.load_data()

        self.model.set_weights(parameters)
        history = self.model.fit(self.x_train, self.y_train, epochs=1, batch_size=128,
                                 validation_data=(self.x_test, self.y_test), verbose=False)
        sample_size = len(self.x_train)
        print(f"Default: {self.cid} -> {history.history['val_accuracy'][-1]}")
        (self.x_train, self.y_train), (self.x_test, self.y_test) = (None, None), (None, None)

        return self.model.get_weights(), sample_size, {"val_accuracy": history.history['val_accuracy'][-1],
                                                       "val_loss": history.history['val_loss'][-1]}

    def fit_fed_prox(self, parameters, config=None):
        _mu = 1.0  # Proximal term 0.01  

        (self.x_train, self.y_train), (self.x_test, self.y_test) = self.load_data()
        self.model.set_weights(parameters)
        batch_size = 128
        num_batches = len(self.x_train) // batch_size

        for epoch in range(1):  # Fixed number of epochs
            batch_count = 0
            for i in range(num_batches):
                x_batch = self.x_train[i * batch_size:(i + 1) * batch_size]
                y_batch = self.y_train[i * batch_size:(i + 1) * batch_size]

                with tf.GradientTape() as tape:

                    if self.model_type == "MLP":
                        x_batch = tf.reshape(x_batch, (-1, 784))

                    predictions = self.model(x_batch, training=True)
                    loss = tf.keras.losses.sparse_categorical_crossentropy(y_batch, predictions)

                    # FedProx term: L2 distance between local and global weights
                    prox_term = sum(
                        tf.reduce_sum(tf.square(w1 - w2)) for w1, w2 in zip(self.model.trainable_weights, parameters))
                    loss = loss + (_mu / 2) * prox_term

                grads = tape.gradient(loss, self.model.trainable_weights)
                self.model.optimizer.apply_gradients(zip(grads, self.model.trainable_weights))

                batch_count += 1
                if batch_count >= num_batches:
                    break

        sample_size = len(self.x_train)
        loss, accuracy = self.evaluate(self.model.get_weights())
        print(f"FedProx: {self.cid} -> {accuracy}")
        (self.x_train, self.y_train), (self.x_test, self.y_test) = (None, None), (None, None)
        return self.model.get_weights(), sample_size, {"val_accuracy": accuracy, "val_loss": loss}

    def fit(self, parameters, config=None):
        if self.scheduler != 'FL':
            return self.fit_fed_prox(parameters, config)            
        else:
            return self.fit_defaut(parameters, config)


    def evaluate(self, parameters):
        (self.x_train, self.y_train), (self.x_test, self.y_test) = self.load_data()
        self.model.set_weights(parameters)
        loss, accuracy = self.model.evaluate(self.x_test, self.y_test, verbose=False)

        (self.x_train, self.y_train), (self.x_test, self.y_test) = (None, None), (None, None)
        return loss, accuracy
