///* Just a little example of how to use Eigen's Levenberg-Marquardt algorithm to fit a non-linear model to some data.
//   This time we'll do Mini-BA i.e. a mini bundle adjustment algorithm with only a few poses - camera and points.
//*/
//
//#include <iostream>
//#include <vector>
//#include <Eigen/Dense>
//#include <unsupported/Eigen/NonLinearOptimization>
//#include <unsupported/Eigen/NumericalDiff> 
//
//struct BundleAdjustment3D
//{
//    typedef double Scalar;
//    // Total parameters: 1 (camera)
//    enum { InputsAtCompileTime = Eigen::Dynamic, ValuesAtCompileTime = Eigen::Dynamic }; // Number of parameters to be optimized and number of observations.
//    typedef Eigen::VectorXd InputType;
//    typedef Eigen::VectorXd ValueType;
//    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> JacobianType;
//
//    // Observations: Where the points were seen in the image
//    const Eigen::VectorXd& observed_pixels; //Layout: [u1, v1, u2, v2, ...] for N points
//    double focal_length = 500.0; // Assume a simple pinhole camera with focal length 1.0
//
//    BundleAdjustment3D(const Eigen::VectorXd& obs) : observed_pixels(obs) {}
//
//    // we pass through the parameters (initial guess) and try to minimise the fvec. 
//    // Model: Pixel = Point_Pos / Camera_Pos (Simplified projection)
//    int operator()(const InputType& params, ValueType& fvec) const
//    {
//        double delta = 1.0; // Huber-like threshold for robust loss
//
//        // params[0, 1, 2] is the camera X Y Z
//        Eigen::Vector3d cam(params[0], params[1], params[2]);
//        int num_points = observed_pixels.size() / 2;
//
//        for (int i = 0; i < num_points; ++i)
//        {
//            // Each point has 3 params: [x, y, z] starting at index 3
//            Eigen::Vector3d p(params[3 + i * 3], params[4 + i * 3], params[5 + i * 3]);
//
//            // Relative position to camera
//            Eigen::Vector3d rel = p - cam;
//
//            // Project 3D to 2D (Pinhole Model)
//            // We assume the camera looks down the Z-axis
//            double pred_u = focal_length * (rel.x() / rel.z());
//            double pred_v = focal_length * (rel.y() / rel.z());
//
//            // Compute Residuals for U and V
//            double res_u = pred_u - observed_pixels[i * 2];
//            double res_v = pred_v - observed_pixels[i * 2 + 1];
//
//            // Apply Robust Huber-like weighting to both U and V
//            fvec[i * 2] = (std::abs(res_u) < delta) ? res_u : (res_u > 0 ? 1 : -1) * std::sqrt(delta * std::abs(res_u));
//            fvec[i * 2 + 1] = (std::abs(res_v) < delta) ? res_v : (res_v > 0 ? 1 : -1) * std::sqrt(delta * std::abs(res_v));
//        }
//
//        // Anchor the camera position to origin [0,0,0]
//        fvec[num_points * 2] = params[0] - 0.0;
//        fvec[num_points * 2 + 1] = params[1] - 0.0;
//        fvec[num_points * 2 + 2] = params[2] - 0.0;
//
//        return 0;
//    }
//
//    int inputs() const { return 3 + (observed_pixels.size() / 2) * 3; }
//    int values() const { return observed_pixels.size() + 3; }
//};
//
//
//int main() {
//    // 1. Two 2D observations (Point1 at [0,0], Point2 at [100, 50])
//    Eigen::VectorXd obs(4);
//    obs << 0.0, 0.0, 100.0, 50.0;
//
//    // 2. Initial Guess: Camera at origin, Points slightly in front (Z=1)
//    // Params: [Cx, Cy, Cz, P1x, P1y, P1z, P2x, P2y, P2z] (Size 9)
//    Eigen::VectorXd params(9);
//    params << 0.1, 0.1, 0.1, 0.0, 0.0, 1.0, 0.2, 0.1, 1.0;
//
//    // 3. Wrap with Numerical Differentiation
//    BundleAdjustment3D functor(obs);
//    Eigen::NumericalDiff<BundleAdjustment3D> normDiff(functor);
//    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<BundleAdjustment3D>> lm(normDiff);
//
//    int info = lm.minimize(params);
//
//    std::cout << "Optimized Cam: " << params.head<3>().transpose() << std::endl;
//    std::cout << "Point 1: " << params.segment<3>(3).transpose() << std::endl;
//    std::cout << "Point 2: " << params.segment<3>(6).transpose() << std::endl;
//
//    return 0;
//}
//
//
//
