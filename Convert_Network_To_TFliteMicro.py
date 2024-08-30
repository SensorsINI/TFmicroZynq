from Auxiliary_Files.TF2TFLite import tf2tf_lite

path_to_models = '../../../Driver/CartPoleSimulation/SI_Toolkit_ASF/Experiments/Trial_14__17_08_2024/Models/'
net_name = 'Dense-7IN-32H1-32H2-1OUT-0'
batch_size = 1

if __name__ == '__main__':
    tf2tf_lite(path_to_models=path_to_models, net_name=net_name, batch_size=batch_size)