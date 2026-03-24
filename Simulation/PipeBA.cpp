//struct PipeBundleAdjustment
//{
//    typedef double Scalar;
//    enum { InputsAtCompileTime = Eigen::Dynamic, ValuesAtCompileTime = Eigen::Dynamic };
//    typedef Eigen::VectorXd InputType;
//    typedef Eigen::VectorXd ValueType;
//
//    const Eigen::VectorXd& observed_pixels;
//    double focal_length;
//    double pipe_radius; // New: Known radius of the pipe in mm/meters
//
//    PipeBundleAdjustment(const Eigen::VectorXd& obs, double f, double r)
//        : observed_pixels(obs), focal_length(f), pipe_radius(r) {
//    }
//
//    int operator()(const InputType& params, ValueType& fvec) const
//    {
//        Eigen::Vector3d cam(params[0], params[1], params[2]);
//        int num_points = observed_pixels.size() / 2;
//
//        for (int i = 0; i < num_points; ++i)
//        {
//            Eigen::Vector3d p(params[3 + i * 3], params[4 + i * 3], params[5 + i * 3]);
//            Eigen::Vector3d rel = p - cam;
//            double z_safe = (rel.z() < 0.1) ? 0.1 : rel.z();
//
//            // 1. Projection Residuals (u, v)
//            fvec[i * 2] = (focal_length * rel.x() / z_safe) - observed_pixels[i * 2];
//            fvec[i * 2 + 1] = (focal_length * rel.y() / z_safe) - observed_pixels[i * 2 + 1];
//
//            // 2. Cylindrical Constraint Residual
//            // Distance from center (x,y) to point should equal pipe_radius
//            // Assuming the pipe runs along the Z-axis
//            double dist_from_center = std::sqrt(p.x() * p.x() + p.y() * p.y());
//            fvec[num_points * 2 + 3 + i] = dist_from_center - pipe_radius;
//        }
//
//        // 3. Camera Anchor (Keep cam at origin)
//        fvec[num_points * 2] = params[0];
//        fvec[num_points * 2 + 1] = params[1];
//        fvec[num_points * 2 + 2] = params[2];
//
//        return 0;
//    }
//
//    int inputs() const { return 3 + (observed_pixels.size() / 2) * 3; }
//    // Values: (2 * N pixels) + (3 cam anchor) + (N cylinder constraints)
//    int values() const {
//        int n = observed_pixels.size() / 2;
//        return (2 * n) + 3 + n;
//    }
//};