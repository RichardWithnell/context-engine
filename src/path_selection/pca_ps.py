from sklearn.decomposition import PCA
from sklearn.datasets import load_iris
from sklearn.preprocessing import StandardScaler
import math
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import pearsonr, betai
from numpy import eye, asarray, dot, sum
from numpy.linalg import svd
import sys

class Parameters(object):
    def __init__(self, name, bandwidth, rtt, loss, jitter, bw_weight=0.25, rtt_weight=0.25, loss_weight=0.25, jitter_weight=0.25):
        self.name = name
        self.bandwidth = bandwidth
        self.bw_weight = bw_weight
        self.rtt = rtt
        self.rtt_weight = rtt_weight
        self.loss = loss
        self.loss_weight = loss_weight
        self.jitter = jitter
        self.jitter_weight = jitter_weight
        self.energy = 0
        self.cost = 0
        self.score = 0


def varimax(Phi, gamma = 1.0, q = 20, tol = 1e-6):
    p,k = Phi.shape
    R = eye(k)
    d=0
    for i in xrange(q):
        d_old = d
        Lambda = dot(Phi, R)
        u,s,vh = svd(dot(Phi.T,asarray(Lambda)**3 - (gamma/p) * dot(Lambda, diag(diag(dot(Lambda.T,Lambda))))))
        R = dot(u,vh)
        d = sum(s)
        if d_old!=0 and d/d_old < 1 + tol: break
    return dot(Phi, R)

def pca_cor(X, do_graph=True):

    # Standardize data
    X_std = StandardScaler().fit_transform(X)

    # Compute the correlation matrix
    cor_mat = np.corrcoef(X.T)

    #print "Correlation Matrix"
    #print cor_mat

    # Eigendecomposition of the correlation matrix
    eig_val_cor, eig_vec_cor = np.linalg.eig(cor_mat)

    # Make a list of (eigenvalue, eigenvector) tuples
    # and sort the (eigenvalue, eigenvector) tuples from high to low
    eig_pairs_cor = [(np.abs(eig_val_cor[i]), eig_vec_cor[:,i]) for i in range(len(eig_val_cor))]
    eig_pairs_cor.sort()
    eig_pairs_cor.reverse()

    return eig_pairs_cor

def calculate_cr(eig_pairs_cor):
    ccr = 0
    contribution_rates = []
    cumulative_contribution_rates = []

    eig_val_sum = sum(e[0] for e in eig_pairs_cor)
    for eig_pair in eig_pairs_cor:
        cr = eig_pair[0] / eig_val_sum
        contribution_rates.append(cr)
        ccr += cr
        cumulative_contribution_rates.append(ccr)

    return contribution_rates, cumulative_contribution_rates

def do_iris():
    data = load_iris()

    X = data.data

    print type(data.data)
    print data.data

    # convert features in column 1 from cm to inches
    X[:,0] /= 2.54
    # convert features in column 2 from cm to meters
    X[:,1] /= 100

    pca_cor(X)


def parameter_list_to_array(params):
    array = []
    for l in params:
        array.append([l.bandwidth, l.rtt, l.loss, l.jitter])
    return array

def find_best_service(contribution_rates, cumulative_contribution_rates, normalized_array, eig_pairs_cor, weights):
    m = 0

    while cumulative_contribution_rates[m] < 0.95:
        m = m + 1

    #
    # Count All Eigen Values Greater Than One
    m = sum(i > 5 for i in [x[0] for x in eig_pairs_cor])

    loadings = []
    overall_scores = []

    for i in range(0, len(normalized_array)):
        Y = []
        #print "Link Name: " +  l_array[i].name + " \n"
        for j in range(0, m + 1):
            y = []
            #print "Principal Component: " + str(j + 1)
            #print ""
            for k in range(0, len(normalized_array[i])):
                #print " + " + str(normalized_array[i][k]) + " * " + str(eig_pairs_cor[j][1][k])
                y.append(eig_pairs_cor[j][1][k] * normalized_array[i][k])
            #print "-----------------"
            #print " = " + str(sum(y))

            Y.append(contribution_rates[j] * sum(y))
            #print "Score: " + str(contribution_rates[j] * sum(y)) + "\n"

        #print "Overall Score: " + str(sum(Y))
        overall_scores.append(sum(Y))
    #print "Scores: " + str(overall_scores)
    net = max(overall_scores)

    return overall_scores

#Vendor Selection Using Principal Component Analysis
def do_example_one():

    normalized_array = [[0.622, 0.261, 0.667, 0.958, 0.100, 0.122],
                        [0.500, 0.333, 0.571, 1.000, 0.200, 0.200],
                        [0.737, 0.429, 0.400, 0.935, 0.133, 0.167],
                        [0.683, 0.286, 0.444, 0.983, 0.182, 1.000],
                        [0.452, 0.353, 0.400, 0.958, 0.400, 0.040],
                        [0.509, 1.000, 0.800, 0.975, 0.167, 0.032],
                        [1.000, 0.500, 0.571, 0.943, 0.333, 0.179],
                        [0.778, 0.667, 0.571, 0.983, 1.000, 0.093],
                        [0.596, 0.176, 0.444, 0.920, 0.167, 0.060],
                        [0.528, 0.545, 1.000, 1.000, 0.222, 0.049]]
    """
    d = [[0.622, 0.261, 0.667, 0.958, 0.100, 0.122],
                        [0.500, 0.333, 0.571, 1.000, 0.200, 0.200],
                        [0.737, 0.429, 0.400, 0.935, 0.133, 0.167],
                        [0.683, 0.286, 0.444, 0.983, 0.182, 1.000],
                        [0.452, 0.353, 0.400, 0.958, 0.400, 0.040],
                        [0.509, 1.000, 0.800, 0.975, 0.167, 0.032],
                        [1.000, 0.500, 0.571, 0.943, 0.333, 0.179],
                        [0.778, 0.667, 0.571, 0.983, 1.000, 0.093],
                        [0.596, 0.176, 0.444, 0.920, 0.167, 0.060],
                        [0.528, 0.545, 1.000, 1.000, 0.222, 0.049]]

    normalized_array = [[d[0][3]/d[0][2], d[0][3]/d[0][0], d[0][4]/d[0][0], d[0][5]/d[0][1], d[0][5]/d[0][0]],
                        [d[1][3]/d[1][2], d[1][3]/d[1][0], d[1][4]/d[1][0], d[1][5]/d[1][1], d[1][5]/d[1][0]],
                        [d[2][3]/d[2][2], d[2][3]/d[2][0], d[2][4]/d[2][0], d[2][5]/d[2][1], d[2][5]/d[2][0]],
                        [d[3][3]/d[3][2], d[3][3]/d[3][0], d[3][4]/d[3][0], d[3][5]/d[3][1], d[3][5]/d[3][0]],
                        [d[4][3]/d[4][2], d[4][3]/d[4][0], d[4][4]/d[4][0], d[4][5]/d[4][1], d[4][5]/d[4][0]],
                        [d[5][3]/d[5][2], d[5][3]/d[5][0], d[5][4]/d[5][0], d[5][5]/d[5][1], d[5][5]/d[5][0]],
                        [d[6][3]/d[6][2], d[6][3]/d[6][0], d[6][4]/d[6][0], d[6][5]/d[6][1], d[6][5]/d[6][0]],
                        [d[7][3]/d[7][2], d[7][3]/d[7][0], d[7][4]/d[7][0], d[7][5]/d[7][1], d[7][5]/d[7][0]],
                        [d[8][3]/d[8][2], d[8][3]/d[8][0], d[8][4]/d[8][0], d[8][5]/d[8][1], d[8][5]/d[8][0]],
                        [d[9][3]/d[9][2], d[9][3]/d[9][0], d[9][4]/d[9][0], d[9][5]/d[9][1], d[9][5]/d[9][0]]]

    """

    X = np.array(normalized_array)
    print "NORMALIZED ARRAY:"
    print X
    scikit_pca(X)

    eig_pairs_cor = pca_cor(X, do_graph=True)

    contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

    print [x[0] for x in eig_pairs_cor]
    print eig_pairs_cor
    print contribution_rates
    print cumulative_contribution_rates

    find_best_service(contribution_rates, cumulative_contribution_rates, normalized_array, eig_pairs_cor)

def do_example_two():

    """
        0.6088, 0.4152, 0.2222, 0.40000, 0.3333, 0.8286
        0.1570, 0.00,   0.00,   0.00,    1.00,   0.8572
        0.6207, 1.00,   0.9444, 0.8500,  1.00,   0.8572
        0.3805, 0.8051, 0.8333, 0.7500,  1.00,   0.00
        0.3829, 0.9025, 0.500,  0.600,   0.00,   0.7143
        0.00,   0.2203, 0.3333, 0.2000,  0.6667, 1.00
        1.00,   0.7076, 1.00,   1.00,    0.6667, 0.2857

    """

    normalized_array = [[0.6088, 0.4152, 0.2222, 0.40000, 0.3333, 0.8286],
                        [0.1570, 0.00, 0.00, 0.00, 1.00, 0.8572],
                        [0.6207, 1.00, 0.9444, 0.8500, 1.00, 0.8572],
                        [0.3805, 0.8051, 0.8333, 0.7500, 1.00, 0.00],
                        [0.3829, 0.9025, 0.500, 0.600, 0.00, 0.7143],
                        [0.00, 0.2203, 0.3333, 0.2000, 0.6667, 1.00],
                        [1.00, 0.7076, 1.00, 1.00, 0.6667, 0.2857]]

    X = np.array(normalized_array)
    print "NORMALIZED ARRAY:"
    print X

    eig_pairs_cor = pca_cor(X, do_graph=False)

    contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

    print eig_pairs_cor
    print contribution_rates
    print cumulative_contribution_rates

    find_best_service(contribution_rates, cumulative_contribution_rates, normalized_array, eig_pairs_cor)

def transform_against_base(base, params):

    bw_std = np.std([x.bandwidth for x in params])
    rtt_std = np.std([x.rtt for x in params])
    loss_std = np.std([x.loss for x in params])
    jitter_std = np.std([x.jitter for x in params])

    bw_avg = sum([x.bandwidth for x in params])/len(params)
    rtt_avg = sum([x.rtt for x in params])/len(params)
    loss_avg = sum([x.loss for x in params])/len(params)
    jitter_avg = sum([x.jitter for x in params])/len(params)

    bw_min = min([x.bandwidth for x in params])
    rtt_min = min([x.rtt for x in params])
    loss_min = min([x.loss for x in params])
    jitter_min = min([x.jitter for x in params])

    bw_max = max([x.bandwidth for x in params])
    rtt_max = max([x.rtt for x in params])
    loss_max = max([x.loss for x in params])
    jitter_max = max([x.jitter for x in params])

    """
    for p in params:
        p.bandwidth = (p.bandwidth - bw_min) / (bw_max - bw_min)
        p.jitter = (jitter_max - p.jitter) / (jitter_max - jitter_min)
        p.loss = (loss_max - p.loss) / (loss_max - loss_min)
        p.rtt = (rtt_max - p.rtt) / (rtt_max - rtt_min)
    """


    for p in params:
        p.bandwidth = abs(base.bandwidth - p.bandwidth)
        p.jitter = abs(base.jitter - p.jitter)
        p.loss = abs(base.loss - p.loss)
        p.rtt = abs(base.rtt - p.rtt)

    for p in params:
        if bw_max == bw_min:
            p.bandwidth = 1
        else:
            p.bandwidth = (bw_max - p.bandwidth) / (bw_max - bw_min)
        if jitter_max == jitter_min:
            p.jitter = 1
        else:
            p.jitter = (jitter_max - p.jitter) / (jitter_max - jitter_min)
        if loss_max == loss_min:
            p.loss = 1
        else:
            p.loss = (loss_max - p.loss) / (loss_max - loss_min)
        if rtt_max == rtt_min:
            p.rtt = 1
        else:
            p.rtt = (rtt_max - p.rtt) / (rtt_max - rtt_min)

    return params

def norm_test(base):
    l1 = Parameters("L1", 7.15, 141.62, 0.2, 91.83)
    l2 = Parameters("L2", 10.72, 65.84, 0.03, 14.94)
    l3 = Parameters("L3", 15.63, 124.64, 0.05, 50.51)
    l4 = Parameters("L4", 2.72, 166.22, 0.26, 55.52)
    l5 = Parameters("L5", 10.72, 46.97, 0.2, 22.80)
    l6 = Parameters("L6", 12.01, 77.31, 0.13, 21.43)
    l7 = Parameters("L7", 6.49, 115.32, 0.14, 31.06)
    l8 = Parameters("L8", 1.19, 348.02, 0.27, 151.28)
    l9 = Parameters("L9", 18.32, 39.92, 0.25, 16.82)
    l10 = Parameters("L10", 1.85, 362.37, 0.10, 135.28)

    l_array = [l1, l2, l3, l4, l5, l6, l7, l8, l9, l10]

    a = parameter_list_to_array(l_array)
    X = np.array(a)
    print "Initial Matrix:"
    print "----------------"
    print X
    print ""

    l_array = transform_against_base(base, l_array)

    a = parameter_list_to_array(l_array)

    X = np.array(a)
    print "Transformed Matrix:"
    print "----------------"
    print X
    print ""

    corr = np.corrcoef(X.T)
    print "Correlation Matrix:"
    print "----------------"
    print corr
    print ""

    cov = np.cov(X.T)
    print "Covariance Matrix:"
    print "----------------"
    print cov
    print ""

    eig_pairs_cor = pca_cor(X, do_graph=False)
    contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

    scores = find_best_service(contribution_rates, cumulative_contribution_rates, X, eig_pairs_cor)

    for i in range(0, len(scores)):
        l_array[i].score = scores[i]

    l_array.sort(key=lambda x: x.score, reverse=True)

    print l_array[0].name + ": " + str(l_array[0].score)

    print ""

def metric_tuple_to_param(param):
    return Parameters(param[0], param[1], param[2], param[3], param[4])

def spec_tuple_to_param(param):
    return Parameters(0, param[0], param[1], param[2], param[3])

def select_path(metrics, spec):

    base = None
    l_array = []

    for m in metrics:
        l_array.append(metric_tuple_to_param(m))

    a = parameter_list_to_array(l_array)
    X = np.array(a)
    print "Initial Matrix:"
    print "----------------"
    print X
    print ""

    if spec is not None:
        base = spec_tuple_to_param(spec)

    if base is not None:
        l_array = transform_against_base(base, l_array)
    else
        l_array = []

    if len(l_array) == 0:
        return 0

    if len(l_array) == 1:
        return l_array[0].name

    a = parameter_list_to_array(l_array)

    try:
        X = np.array(a)
        print "Transformed Matrix:"
        print "----------------"
        print X
        print ""

        corr = np.corrcoef(X.T)
        print "Correlation Matrix:"
        print "----------------"
        print corr
        print ""

        eig_pairs_cor = pca_cor(X, do_graph=False)
        contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

        scores = find_best_service(contribution_rates, cumulative_contribution_rates, X, eig_pairs_cor, (0,0,0,0,0))

        for i in range(0, len(scores)):
            l_array[i].score = scores[i]

        l_array.sort(key=lambda x: x.score, reverse=True)

        return l_array[0].name
    except Exception as e:
        print "PCA Error: %s" % (e.message, )
        return 0

def do_tech_report(base):
    l1 = Parameters("L1", 7.15, 141.62, 0.2, 91.83)
    l2 = Parameters("L2", 10.72, 65.84, 0.03, 14.94)
    l3 = Parameters("L3", 15.63, 124.64, 0.05, 50.51)
    l4 = Parameters("L4", 2.72, 166.22, 0.26, 55.52)
    l5 = Parameters("L5", 10.72, 46.97, 0.2, 22.80)
    l6 = Parameters("L6", 12.01, 77.31, 0.13, 21.43)
    l7 = Parameters("L7", 6.49, 115.32, 0.14, 31.06)
    l8 = Parameters("L8", 1.19, 348.02, 0.27, 151.28)
    l9 = Parameters("L9", 18.32, 39.92, 0.25, 16.82)
    l10 = Parameters("L10", 1.85, 362.37, 0.10, 135.28)

    l_array = [l1, l2, l3, l4, l5, l6, l7, l8, l9, l10]
    a = parameter_list_to_array(l_array)

    X = np.array(a)
    print "Initial ARRAY:"
    print X

    """
    [[3.5700000000000003, 94.65, 0.0, 69.03],
     [0.0, 18.870000000000005, 0.17, 7.860000000000001],
     [4.91, 77.67, 0.15000000000000002, 27.709999999999997],
     [8.0, 119.25, 0.06, 32.72],
     [0.0, 0.0, 0.0, 0.0],
     [1.2899999999999991, 30.340000000000003, 0.07, 1.370000000000001],
     [4.23, 68.35, 0.06, 8.259999999999998],
     [9.530000000000001, 301.04999999999995, 0.07, 128.48],
     [7.6, 7.049999999999997, 0.04999999999999999, 5.98],
     [8.870000000000001, 315.4, 0.1, 112.48]]
    """
    l_array = transform_against_base(base, l_array)

    a = parameter_list_to_array(l_array)

    X = np.array(a)
    print "Transformed ARRAY:"
    print X

    #normalize_list = normalize_parameter_list_trans(l_array)
    normalized_array = parameter_list_to_array(l_array)

    X = np.array(normalized_array)
    print "NORMALIZED ARRAY:"
    print X


    eig_pairs_cor = pca_cor(X, do_graph=False)

    contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

    print eig_pairs_cor
    print contribution_rates
    print cumulative_contribution_rates

    scores = find_best_service(contribution_rates, cumulative_contribution_rates, normalized_array, eig_pairs_cor)

    for i in range(0, len(scores)):
        l_array[i].score = scores[i]

    l_array.sort(key=lambda x: x.score, reverse=True)

    for l in l_array:
        print l.name + ": " + str(l.score)

if __name__ == '__main__':

    np.set_printoptions(precision=3, suppress=True)
    """
    L8: 0.233106065746
    L10: 0.217144550172
    L1: -0.454060185343
    L4: -0.48848568804
    L2: -0.671104593206
    L5: -0.693760084519
    L7: -0.713020591138
    L3: -0.74590031021
    L6: -0.752971008829
    L9: -0.78150113449

    """
    base = Parameters("Voice", 0.128, 70, 0.01, 30)
    norm_test(base)
    #do_tech_report(base)

    """
    L2: 1.04269993842
    L7: 1.00383548623
    L5: 0.939871347093
    L6: 0.906834196628
    L1: 0.835838417013
    L3: 0.725159219322
    L4: 0.630702341646
    L9: 0.512305560162
    L10: 0.368445557672
    L8: 0.183416563098

    """
    base = Parameters("Video", 8.00, 50, 0.0001, 30)
    norm_test(base)

    """
    L10: 1.24720449488
    L8: 1.22785087559
    L4: 0.682184216463
    L1: 0.660829903112
    L7: 0.493804296376
    L3: 0.361178333158
    L2: 0.315398516271
    L6: 0.279518293705
    L5: 0.250373204138
    L9: 0.0125895347573
    """
    base = Parameters("Sensor", 0.01, 500.00, 0.00, 500.00)
    norm_test(base)

    """
    L10: 1.24720449488
    L8: 1.22785087559
    L4: 0.682184216463
    L1: 0.660829903112
    L7: 0.493804296376
    L3: 0.361178333158
    L2: 0.315398516271
    L6: 0.279518293705
    L5: 0.250373204138
    L9: 0.0125895347573
    """
    base = Parameters("High Bw", 0.00, 00.00, 0.00, 0.00)
    norm_test(base)
