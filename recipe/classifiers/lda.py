# -*- coding: utf-8 -*-

"""
Copyright 2016 Walter José and Alex de Sá

This file is part of the RECIPE Algorithm.

The RECIPE is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your option)
any later version.

RECIPE is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. See http://www.gnu.org/licenses/.

"""

from sklearn.discriminant_analysis import LinearDiscriminantAnalysis

#Ignoring the warnings:
import warnings
warnings.filterwarnings("ignore")

def lda(args):

	"""Uses scikit-learn's LinearDiscriminantAnalysis, a classifier with a quadratic decision boundary, generated by fitting class conditional densities to the data and using Bayes’ rule. 
    
    Parameters
    ----------
  
    solver : ‘svd’, ‘lsqr’, ‘eigen’
		Solver to use, possible values

    tol : float
		Threshold used for rank estimation in SVD solver.

	store_covariance : bool
		Additionally compute class covariance matrix (default False).

    """

	solv = args[1]

	t = float(args[2])

	store_cov = False
	if(args[3].find("True")!=-1):
		store_cov = True

	return LinearDiscriminantAnalysis(solver=solv, shrinkage=None, priors=None, n_components=None, 
		store_covariance=store_cov, tol=t)