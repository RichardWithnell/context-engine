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

def pca_cor(X, do_graph=True):

    # Standardize data

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

    print "-------------"
    print "Eigen Value CR CCR"
    eig_val_sum = sum(e[0] for e in eig_pairs_cor)
    for eig_pair in eig_pairs_cor:
        cr = eig_pair[0] / eig_val_sum
        contribution_rates.append(cr)
        ccr += cr
        cumulative_contribution_rates.append(ccr)

        print str(eig_pair[0]) + "\t" + str(cr) + "\t" + str(ccr)
    print "-------------"
    return contribution_rates, cumulative_contribution_rates

def parameter_list_to_array(params, weight=(1,1,1,1)):
    array = []
    for l in params:
        a = []
        if weight[0] == 1:
            a.append(l.bandwidth)
        if weight[1] == 1:
            a.append(l.rtt)
        if weight[2] == 1:
            a.append(l.loss)
        if weight[3] == 1:
            a.append(l.jitter)

        array.append(a)
    return array

def find_best_service(contribution_rates, cumulative_contribution_rates, normalized_array, eig_pairs_cor, weights):
    m = 0

    while cumulative_contribution_rates[m] <= 0.95:
        m = m + 1


    #Count All Eigen Values Greater Than One
    #m = sum(i > 1 for i in [x[0] for x in eig_pairs_cor])

    print "m: " + str(m)

    loadings = []
    overall_scores = []

    for i in range(0, len(normalized_array)):
        Y = []
        #print "Link Name: " +  l_array[i].name + " \n"
        for j in range(0, m + 1):
            y = []
            #print "Principal Component: " + str(j + 1)
            #print "-----------------"
            #print " + "
            for k in range(0, len(normalized_array[i])):
                #print " + " + str(normalized_array[i][k]) + " * " + str(eig_pairs_cor[j][1][k]) + " * " + str(weights[k])

                y.append(eig_pairs_cor[j][1][k] * normalized_array[i][k])
            #print "-----------------"
            #print " = " + str(sum(y))

            Y.append(contribution_rates[j] * sum(y))
            #print "Score: " + str(contribution_rates[j] * sum(y)) + "\n"
        print str(Y) + " " + str(abs(sum(Y)))
        #print "Overall Score: " + str(sum(Y))
        overall_scores.append(abs(sum(Y)))
    #print "Scores: " + str(overall_scores)
    net = max(overall_scores)


    return overall_scores

def transform_against_base(base, params):

    """
    for p in params:
        p.bandwidth = (p.bandwidth - bw_min) / (bw_max - bw_min)
        p.jitter = (jitter_max - p.jitter) / (jitter_max - jitter_min)
        p.loss = (loss_max - p.loss) / (loss_max - loss_min)
        p.rtt = (rtt_max - p.rtt) / (rtt_max - rtt_min)
    """

    if base is not None:
        for p in params:
            p.bandwidth = abs(base.bandwidth - p.bandwidth)
            p.jitter = abs(base.jitter - p.jitter)
            p.loss = abs(base.loss - p.loss)
            p.rtt = abs(base.rtt - p.rtt)


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


    for p in params:
        if bw_max == bw_min:
            p.bandwidth = 1
        else:
            if base is None:
                print "Base Is None"
                p.bandwidth = (p.bandwidth - bw_min) / (bw_max - bw_min)
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

def validate_array(base, l_array):

    bw_insufficient = 0
    rtt_insufficient = 0
    loss_insufficient = 0
    jitter_insufficient = 0

    for l in l_array:
        if l.bandwidth < base.bandwidth:
            bw_insufficient = 1
        if l.rtt > base.rtt:
            rtt_insufficient = 1
        if l.loss > base.loss:
            loss_insufficient = 1
        if l.jitter > base.jitter:
            jitter_insufficient = 1

    weights = [bw_insufficient, rtt_insufficient, loss_insufficient, jitter_insufficient]

    return weights

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

    l_array = transform_against_base(base, l_array)


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

        scores = find_best_service(contribution_rates, cumulative_contribution_rates, X, eig_pairs_cor, (1,1,1,1))

        for i in range(0, len(scores)):
            l_array[i].score = scores[i]

        l_array.sort(key=lambda x: x.score, reverse=True)
        for l in l_array:
            print  l.name + ": " + str(l.score)
        return l_array[0].name
    except Exception as e:
        print "PCA Error: %s" % (e.message, )
        return 0

def norm_test(base, weight=0, transform=1):
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

    weight = (1,1,1,1)

    #if base != None:
    #   weight = validate_array(base, l_array)

    l_array = transform_against_base(base, l_array)

    a = parameter_list_to_array(l_array)

    X = np.array(a)
    print "Initial Matrix:"
    print "----------------"
    print X
    print ""

    a = parameter_list_to_array(l_array)

    X = np.array(a)
    print "Initial Matrix:"
    print "----------------"
    print X
    print ""


    X = np.array(a)
    """
    print "Transformed Matrix:"
    print "----------------"
    print X
    print ""
    """
    corr = np.corrcoef(X.T)
    print "Correlation Matrix:"
    print "----------------"
    print corr
    print ""
    """
    cov = np.cov(X.T)
    print "Covariance Matrix:"
    print "----------------"
    print cov
    print ""
    """
    eig_pairs_cor = pca_cor(X, do_graph=False)
    contribution_rates, cumulative_contribution_rates = calculate_cr(eig_pairs_cor)

    print eig_pairs_cor

    print "LOADINGS"
    print "--------------"
    loadings = []
    for e in eig_pairs_cor:
        loadings.append(e[1])

    L = np.array(loadings)

    print L.T

    scores = find_best_service(contribution_rates, cumulative_contribution_rates, X, eig_pairs_cor, (1,1,1,1))

    for i in range(0, len(scores)):
        l_array[i].score = scores[i]

    l_array.sort(key=lambda x: x.score, reverse=True)

    for l in l_array:
        print  l.name + ": " + str(l.score)

    return l_array[0].name + ": " + str(l_array[0].score)

if __name__ == '__main__':
    print select_path((("L4", 2.6825, 175.629, 0.00, 37.829),
                 ("L5", 5.73, 58.00, 0.00, 24.00),
                 ("L7", 7.27, 118.089, 0.00, 18.692),
                 ("L8", 2.08, 176.00, 0.00, 81.00),
                 ("L10",1.82, 342, 2.00, 105.23)), None)

    print select_path((("L4", 2.6825, 175.629, 0.00, 37.829),
                 ("L5", 8.802, 46.00, 0.00, 15.00),
                 ("L7", 7.27, 118.089, 0.00, 18.692),
                 ("L8", 2.08, 176.00, 0.00, 81.00),
                 ("L10",1.82, 342, 2.00, 105.23)), (8.00, 50.00, 0.010, 30.000))

    print select_path((("L4", 2.6825, 175.629, 0.00, 37.829),
                 ("L5", 8.802, 46.00, 0.00, 15.00),
                 ("L7", 7.27, 118.089, 0.00, 18.692),
                 ("L8", 2.08, 176.00, 0.00, 81.00),
                 ("L10",1.82, 342, 2.00, 105.23)), (0.010, 300.00, 0.00, 300.000))

    sys.exit(1)

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
    r = norm_test(base)
    print "VOICE: " + r
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
    base = Parameters("Video", 8.00, 50, 0.01, 30)
    r = norm_test(base)
    print "VIDEO: " + r

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
    base = Parameters("Sensor", 0.01, 300.00, 0.00, 300.00)
    r = norm_test(base)
    print "Sensor: " + r

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
    r = norm_test(None)
    print "High Bw: " + r
