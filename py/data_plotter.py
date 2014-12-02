import h5py
import numpy as np
import sys
import matplotlib.pyplot as plt
import mayavi.mlab as mlab

def plot2d(facets, data):
    x_index = [0, 2]
    y_index = [1, 3]
    vertices = np.array([
        facets[:, x_index].flatten(),
        facets[:, y_index].flatten()
    ]).T

    # theta = np.linspace(0, 2 * np.pi, 1000)
    # x = 5 + 3 * np.cos(theta)
    # y = 3 * np.sin(theta)
    # n_mag = np.sqrt((5 - x) ** 2 + y ** 2)
    # dy = 1.0 / (x * (1 + (y * y / (x * x))))
    # dx = (-y / x) * dy
    # exact = (5 - x) * dx - y * dy
    # exact /= n_mag;
    # plt.plot(x, exact, 'r')
    x = np.linspace(-10, 10, 1000)
    s = 1
    uz = s / (2 * np.pi) * np.arctan(1.0 / x)
    plt.plot(x, uz, 'r.-')
    plt.plot(vertices[:, 0], data, 'b.-')
    plt.show()

def plot3d(facets, data):
    x_index = [0, 3, 6]
    y_index = [1, 4, 7]
    z_index = [2, 5, 8]
    vertices = np.array([
        facets[:, x_index].flatten(),
        facets[:, y_index].flatten(),
        facets[:, z_index].flatten()
    ]).T

    n_v = vertices.shape[0]
    faces = np.arange(n_v).reshape((n_v / 3, 3))

    mlab.triangular_mesh(vertices[:, 0], vertices[:, 1], vertices[:, 2],
                       faces[:,:], scalars = data)
    mlab.show()

def main(filename, values_dim):
    f = h5py.File(filename)
    facets = f['facets']
    data = f['values'][:, values_dim]
    if (facets.shape[1] == 9):
        plot3d(facets, data)
    elif (facets.shape[1] == 4):
        plot2d(facets, data)


if __name__ == "__main__":
    advice = "Usage is: python py/data_plotter.py filename column" +\
             "\n where the column specifies the column of the values" +\
             " dataspace to display"
    if len(sys.argv) < 3:
        print(advice)
        sys.exit()
    filename = sys.argv[1]
    try:
        values_dim = int(sys.argv[2])
    except:
        print(advice)
    main(sys.argv[1], int(sys.argv[2]))
