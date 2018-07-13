""" Calculates the positioning uncertainty when calculating the
    position of the pair of Wi-Fi modules on the Wi-Fi scanner. """

import numpy as np

# Distances
dab = 490
dab_u = 10
dab_pu = dab_u / dab
da1 = 180
da1_u = 10
da2 = 340
da2_u = 10

# Scanner orientation
dir = np.array([0.0001, 0.0001, 100])
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

dva1 = uab * da1
dva1_u = (uab_u / uab + da1_u / da1) * dva1
p1_u = pa_u + dva1_u
print("Module 1's position uncertainty", p1_u)

dva2 = uab * da2
dva2_u = (uab_u / uab + da2_u / da2) * dva2
p2_u = pa_u + dva2_u
print("Module 2's position uncertainty", p2_u)
