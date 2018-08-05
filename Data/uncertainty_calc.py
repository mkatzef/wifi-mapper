""" Calculates the positioning uncertainty when calculating the
    position of the pair of Wi-Fi modules on the Wi-Fi scanner. """

import numpy as np

# Distances
dab = 490
dab_u = 10
dab_pu = dab_u / dab

das = np.array([110, 185, 255, 325, 390])
das_u = 5
dbs = dab - das
dbs_u = 5

# Scanner orientation
dir = np.array([0.0001, 0.0001, 10000.0001])
dir = dir / np.linalg.norm(dir)
print("Scanner rotation:", dir)
diff = dab * dir
pa_u = np.array([2.787, 2.603, 5.653])
pb_u = np.array([3.653, 2.944, 7.919])
diff_u = pa_u + pb_u

# Uncertainty propagation calculation
num = diff
num_u = diff_u
sqrsum = sum(diff ** 2)
sqrsum_u = sum(2 * (diff_u / diff) * (diff ** 2))
denom = sqrsum ** (1/2)
denom_u = (1/2) * (sqrsum_u / sqrsum) * denom
uab = num / denom
uab_u = ((num_u / num) + (denom_u / denom)) * uab

for i in range(len(das)):
    dai = das[i]
    dbi = dbs[i]
    
    dvai = uab * dai
    dvai_u = (uab_u / uab + das_u / dai) * dvai
    pi_a_u = pa_u + dvai_u
    print("Uncertainty for p{} using dai:".format(i + 1), pi_a_u)
    
    dvbi = uab * dbi
    dvbi_u = (uab_u / uab + dbs_u / dbi) * dvbi
    pi_b_u = pb_u + dvbi_u
    print("Uncertainty for p{} using dbi:".format(i + 1), pi_b_u)
